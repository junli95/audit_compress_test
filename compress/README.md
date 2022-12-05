Compress and store audit log file.
compress\_xx\_entry.c means compressing per entry of audit log.
compress\_xx.c is implemented by per chunk (e.g., 1MB).

```
make
./gen-log < ../audit.log.example_11.18 | ./compress-lz4
diff audit.log.origin audit.log.decompress
./gen-log < ../audit.log.example_11.18 | ./compress-zstd
diff audit.log.origin audit.log.decompress
```
