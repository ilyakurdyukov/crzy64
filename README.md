# crzy64
### An easy to decode base64 modification. 

This is a base64 modification designed to simplify the decoding step.

For the four encoded bytes, it only takes 12 operations (+ - & ^ << >>) to convert them to 24 bits of data. Using 64-bit or vector instructions makes decoding even more efficient.

But it's bound to ASCII character codes and will not work with other encodings like EBCDIC.

There is a difference with base64 as it uses "./" instead of "+/" and the data is also pre-shuffled to speed up decoding.

### Build

    $ make all check

### Benchmark

* "size" refers to processing of that amount of data between time measurements. 

* "unaligned" for tests means processing data from unaligned pointers. 

* "block repeat" means repeating processing of the same block of a specified size until the total "size" is reached.

```
$ make bench
./crzy64_bench
vector: avx2, fast64: yes, unaligned: yes
size: 100 MB

memcpy: 7.911ms (12640.63 MB/s)
encode: 16.540ms (6045.95 MB/s)
decode: 14.586ms (6855.89 MB/s)
encode_unaligned: 16.514ms (6055.47 MB/s)
decode_unaligned: 14.579ms (6859.18 MB/s)

block repeat:
memcpy (1Kb): 1.115ms (89686.10 MB/s)
encode (1Kb): 14.567ms (6864.83 MB/s)
decode (1Kb): 6.557ms (15250.88 MB/s)
memcpy (10Kb): 1.443ms (69300.07 MB/s)
encode (10Kb): 12.308ms (8124.80 MB/s)
decode (10Kb): 5.708ms (17519.27 MB/s)
memcpy (100Kb): 2.394ms (41771.09 MB/s)
encode (100Kb): 12.307ms (8125.46 MB/s)
decode (100Kb): 7.094ms (14096.42 MB/s)
memcpy (1Mb): 3.212ms (31133.25 MB/s)
encode (1Mb): 12.501ms (7999.36 MB/s)
decode (1Mb): 7.578ms (13196.09 MB/s)
memcpy (10Mb): 7.053ms (14178.36 MB/s)
encode (10Mb): 16.046ms (6232.08 MB/s)
decode (10Mb): 13.656ms (7322.79 MB/s)
memcpy (20Mb): 7.594ms (13168.29 MB/s)
encode (20Mb): 16.217ms (6166.37 MB/s)
decode (20Mb): 14.173ms (7055.67 MB/s)
```

