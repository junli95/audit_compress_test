import sys
import torch
import torch.nn as nn
import torch.nn.functional as F
from tqdm import tqdm
from collections import defaultdict

class LSTMAutoencoder(nn.Module):
    def __init__(self, input_size, hidden_size, num_layers):
        super(LSTMAutoencoder, self).__init__()
        self.input_size = input_size
        self.hidden_size = hidden_size
        self.num_layers = num_layers
        
        self.encoder = nn.LSTM(input_size, hidden_size, num_layers, batch_first=True)
        self.decoder = nn.LSTM(hidden_size, input_size, num_layers, batch_first=True)
        
    def forward(self, x):
        # x: (batch_size, seq_len, input_size)
        encoded, _ = self.encoder(x)
        # encoded: (batch_size, seq_len, hidden_size)
        decoded, _ = self.decoder(encoded)
        # decoded: (batch_size, seq_len, input_size)
        return encoded, decoded

# Load audit log data
def read_logs(log_file):
    logs = []
    with open(log_file) as f:
        for line in f:
            log = line.strip().split()
            logs.append(log)
    return logs


# log_file = 'audit_selected.log.normal.noblank.txt'
log_file = sys.argv[1]
logs = read_logs(log_file)
max_events = 1000  # maximum number of unique events to consider

event_counts = defaultdict(int)
grouped_logs = defaultdict(list)  # group logs by log[22]

# print_count=0
for log in logs:
    # print_count+=1
    # print(print_count)
    group = log[1]  # use log[22] as the group key
    grouped_logs[group].append(log)
    for event in log:
        event_counts[event] += 1
# Select the top `max_events` events by frequency
top_events = sorted(event_counts, key=lambda k: event_counts[k], reverse=True)[:max_events]
print('top_events:',len(top_events))
event_to_idx = {event: i for i, event in enumerate(top_events)}

seq_len = 25*30  # 24 sequence length 10
batch_size = 32  #32
num_features = min(len(top_events), max_events)  # added number limit for one-hot encoding

first_flag = 0

# One-hot encode the logs
x = []
all_logs = [log for group_logs in grouped_logs.values() for log in group_logs]
for j in range(0, len(all_logs), seq_len):
    log_slice = all_logs[j:j+seq_len]
    log_onehot = torch.zeros(seq_len, num_features)
    for i, event in enumerate(log_slice):
        event_tuple = tuple(event)
        if event_tuple in event_to_idx and event_to_idx[event_tuple] < num_features:
            log_onehot[i, event_to_idx[event]] = 1
    x.append(log_onehot.unsqueeze(0))
x = torch.cat(x, dim=0)  # x: (num_logs, seq_len, num_features)

# Create the autoencoder model
input_size = num_features
hidden_size = 64    #64 32
num_layers = 2

model = LSTMAutoencoder(input_size, hidden_size, num_layers)

# Train the model
optimizer = torch.optim.Adam(model.parameters(), lr=0.001)
criterion = nn.MSELoss()

num_epochs = 1
device = torch.device('cpu')
for epoch in range(num_epochs):
    for i in range(0, len(x), batch_size):
        batch = x[i:i+batch_size].to(device)
        optimizer.zero_grad()
        encoded, decoded = model(batch)
        loss = criterion(decoded, batch)
        loss.backward()
        optimizer.step()

        if i%(batch_size*1000)==0 or i==len(x):
            print(f"Epoch [{epoch+1}/{num_epochs}], Step [{i+1}/{len(x)}], Loss: {loss.item():.8f}")

# After training the model
torch.save(model.state_dict(), 'trained_model.pth')

print("enter test read log")
# test_log_file = 'audit_selected.log.anomaly.noblank.txt' #audit_selected.log.anomaly.noblank.part.txt
test_log_file = sys.argv[2]
test_logs = read_logs(test_log_file)

# One-hot encode the logs
print("enter encode")
test_event_counts = defaultdict(int)
for test_log in test_logs:
    for event in test_log:
        test_event_counts[event] += 1
# Select the top `max_events` events by frequency
test_top_events = sorted(test_event_counts, key=lambda k: test_event_counts[k], reverse=True)[:max_events]
print('test_top_events:',len(test_top_events))
event_to_idx = {event: i for i, event in enumerate(test_top_events)}

threshold = 0.005  # can adjust
print("enter anomalous detection")
anomalous_indices = []

with torch.no_grad():
    for i in range(len(test_logs) // seq_len):
        start = i * seq_len
        end = (i + 1) * seq_len
        x_batch = []
        for j in range(start, end):
            log_slice = test_logs[j]
            log_onehot = torch.zeros(seq_len, num_features)
            for k, event in enumerate(log_slice):
                if event in event_to_idx and event_to_idx[event] < num_features:
                    log_onehot[k, event_to_idx[event]] = 1
            x_batch.append(log_onehot.unsqueeze(0))
        
        x_batch = torch.cat(x_batch, dim=0).to(device)
        
        # Define the fully connected network
        class FCNet(nn.Module):
            def __init__(self, input_size, hidden_size, output_size):
                super(FCNet, self).__init__()
                self.fc1 = nn.Linear(input_size, hidden_size)
                self.fc2 = nn.Linear(hidden_size, output_size)
            
            def forward(self, x, input_size):
                x = x.view(-1, input_size)
                x = F.relu(self.fc1(x))
                x = self.fc2(x)
                return x

        # Pass the current batch through the fully connected network and compute the reconstruction error
        model = FCNet(seq_len*num_features, 64, seq_len*num_features)
        model.to(device)
        encoded_batch = model(x_batch.flatten(start_dim=0, end_dim=1), seq_len*num_features)
        decoded_batch = encoded_batch.view(-1, seq_len, num_features)
        mse_loss = torch.mean(torch.square(decoded_batch - x_batch), dim=(1, 2))
        batch_anomalous_indices = torch.where(mse_loss > threshold)[0].tolist()
        batch_anomalous_indices = [k + start for k in batch_anomalous_indices if k + start < len(test_logs)]
        anomalous_indices += batch_anomalous_indices
        
        # Free the space
        del x_batch
        del encoded_batch
        del decoded_batch
        torch.cuda.empty_cache()

anomalous_logs = [test_logs[i] for i in anomalous_indices]

# Print the results
print(f"Total test logs: {len(test_logs)}")
print(f"Num anomalous logs detected: {len(anomalous_logs)}")

# print("Anomalous logs:")
# for log in anomalous_logs:
#     print(log)

# non_anomalous_indices = [i for i in range(len(test_logs)) if i not in anomalous_indices]
# non_anomalous_logs = [test_logs[i] for i in non_anomalous_indices]

# file = open("anomaly_nodetect.txt", "w")

# # Print the non-anomalous logs
# # print("Non-anomalous logs:")
# for log in non_anomalous_logs:
#     # print(log)
#     file.write(log)
    

