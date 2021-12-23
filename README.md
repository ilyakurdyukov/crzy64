# crzy64
### An easy to decode base64 modification. 

This is a base64 modification designed to simplify the decoding step.

For the four encoded bytes, it only takes 12 operations (+ - & ^ << >>) to convert them to 24 bits of data. Using 64-bit or vector instructions makes decoding even more efficient.

But it's bound to ASCII character codes and will not work with other encodings like EBCDIC.

There is a difference with base64 as it uses "./" instead of "+/" and the data is also pre-shuffled to speed up decoding.

### Build

    $ make all check

### Benchmark

    $ make bench
    ./crzy64_bench
    vector: avx2
    fast64: yes
    size: 100 MB
    
    memcpy: 7.911ms (12640.63 MB/s)
    encode: 26.173ms (3820.73 MB/s)
    decode: 14.586ms (6855.89 MB/s)
    encode_unaligned: 26.635ms (3754.46 MB/s)
    decode_unaligned: 14.579ms (6859.18 MB/s)

