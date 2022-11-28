Plugin for storing the audit log file

```
make plugin-new
sudo bash
export LD_LIBRARY_PATH=:/usr/local/lib
auditctl -a always,exit -S all
auditd -f -n -s enable -c /usr/local/etc/audit >& /dev/null
auditctl -l
auditctl -D
```
```
ps -aux | grep audit
sudo kill PID 
```
