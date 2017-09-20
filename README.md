## Overview

logcabin-redis-proxy is the proxy that supports redis compatible protocol and working on LogCabin cluster.

## Getting started

### Compile logcabin-redis-proxy

- make sure your g++ supports C++11;

- run the following command:

      $ git submodule update --init # download submodule simple_resp 
      $ mkdir build && cd build
      $ cmake ..
      $ make -j`nproc`
      
### Run logcabin-redis-proxy

- First, run your [logcabin](https://github.com/tigerzhang/logcabin) cluster:

      $ ./LogCabin --config ./logcabin-1.conf --bootstrap
      $ ./LogCabin --config ./logcabin-1.conf  # default running on :5254
      
- Then, run logcabin-redis-proxy

      $ ./logcabin_redis_proxy -c localhost:5254 -s 1024 -p 6381 -a "127.0.0.1"
      
- Try redis command

      $ redis-cli -p 6380
      127.0.0.1:6380> rpush foo 1 2 3 4 5 6 7 8 9
      OK
      127.0.0.1:6380> LRANGE foo 0 -1
      1) "1"
      2) "2"
      3) "3"
      4) "4"
      5) "5"
      6) "6"
      7) "7"
      8) "8"
      9) "9"
      127.0.0.1:6380> LTRIM foo 3
      OK
      127.0.0.1:6380> LRANGE foo
      1) "1"
      2) "2"
      3) "3"
      
## TODO

The project is woking on the following feature:

- High performance and asynchronous;

- Support more redis-like command;

- More test cases(both unit test and integrated test);

- Key sharding

- Add more log;
