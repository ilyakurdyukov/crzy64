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
    
    memcpy: 8.592ms
    memcpy: 8.562ms
    memcpy: 8.565ms
    memcpy: 8.542ms
    memcpy: 9.144ms
    
    encode: 28.767ms
    encode: 28.806ms
    encode: 28.818ms
    encode: 28.572ms
    encode: 28.627ms
    
    decode: 16.798ms
    decode: 16.410ms
    decode: 16.526ms
    decode: 16.701ms
    decode: 16.286ms

