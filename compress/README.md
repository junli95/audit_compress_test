Compress and store audit log file
```
make
./gen-log < audit.log.example_11.18 | ./compress-lz4
./gen-log < audit.log.example_11.18 | ./compress-zstd
```
