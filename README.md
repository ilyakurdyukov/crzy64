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
    vector: ssse3
    fast64: yes
    size: 100 MB
    
    memcpy: 8.187ms (12214.49 MB/s)
    memcpy: 8.301ms (12046.74 MB/s)
    memcpy: 8.311ms (12032.25 MB/s)
    memcpy: 8.213ms (12175.82 MB/s)
    memcpy: 8.648ms (11563.37 MB/s)
    
    encode: 28.821ms (3469.69 MB/s)
    encode: 28.781ms (3474.51 MB/s)
    encode: 28.755ms (3477.66 MB/s)
    encode: 29.022ms (3445.66 MB/s)
    encode: 28.906ms (3459.49 MB/s)
    
    decode: 16.884ms (5922.77 MB/s)
    decode: 16.118ms (6204.24 MB/s)
    decode: 16.799ms (5952.74 MB/s)
    decode: 16.557ms (6039.74 MB/s)
    decode: 16.536ms (6047.41 MB/s)


