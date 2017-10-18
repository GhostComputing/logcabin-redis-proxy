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

      $ ./logcabin_redis_proxy -c localhost:5254 -s 1024 -p 6380 -a 127.0.0.1 -t 20 # run 20 threads
      
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
      127.0.0.1:6380> LTRIM foo 0 3
      OK
      127.0.0.1:6380> LRANGE foo 0 -1
      1) "1"
      2) "2"
      3) "3"

## Run integrated test

- make sure logcabin cluster and logcabin_redis_proxy is running;

- make sure your golang working environment(the test code depends on [go-redis](https://github.com/go-redis/redis))

- execuate following command(suppose your logcabin cluster and proxy are all running at '192.168.2.109' address):

      $ go run test/integrated_test_of_proxy.go --cluster=192.168.2.109:5254 --proxy=192.168.2.109:6380
      => TEST rpush command successfully
      => TEST ltrim command successfully
      => TEST lrange command successfully

## Benchmark

- Environment

    - OS: Linux ubuntu 3.19.0-25-generic x86_64 GNU/Linux
    
    - CPU: 16
    
    - Memory: 32G
    
- Run LogCabin benchmark
    
        $ ./Benchmark -c $LOCALSERVER --threads 50 --writes 10000
        Benchmark took 632.791 ms to write 10000 objects OPS: 15803
        
- Run redis benchmark

        $./logcabin_redis_proxy ./logcabin_redis_proxy -c localhost:5254 -s 10240 -p 6380 -a 127.0.0.1 -t 50
        
        $ redis-benchmark -p 6380 -t rpush -c 200 -n 10000 -r 100000000
        ====== RPUSH ======
          10000 requests completed in 0.63 seconds
          200 parallel clients
          3 bytes payload
          keep alive: 1
        
        0.01% <= 1 milliseconds
        0.66% <= 2 milliseconds
        14.56% <= 3 milliseconds
        20.93% <= 4 milliseconds
        26.90% <= 5 milliseconds
        34.24% <= 6 milliseconds
        38.11% <= 7 milliseconds
        44.64% <= 8 milliseconds
        48.50% <= 9 milliseconds
        53.03% <= 10 milliseconds
        56.70% <= 11 milliseconds
        60.24% <= 12 milliseconds
        63.82% <= 13 milliseconds
        66.63% <= 14 milliseconds
        69.67% <= 15 milliseconds
        72.24% <= 16 milliseconds
        74.69% <= 17 milliseconds
        77.06% <= 18 milliseconds
        79.05% <= 19 milliseconds
        80.86% <= 20 milliseconds
        82.79% <= 21 milliseconds
        84.25% <= 22 milliseconds
        85.91% <= 23 milliseconds
        87.11% <= 24 milliseconds
        88.32% <= 25 milliseconds
        89.64% <= 26 milliseconds
        90.68% <= 27 milliseconds
        91.63% <= 28 milliseconds
        92.55% <= 29 milliseconds
        93.34% <= 30 milliseconds
        94.06% <= 31 milliseconds
        94.68% <= 32 milliseconds
        95.37% <= 33 milliseconds
        95.80% <= 34 milliseconds
        96.35% <= 35 milliseconds
        96.81% <= 36 milliseconds
        97.16% <= 37 milliseconds
        97.56% <= 38 milliseconds
        97.85% <= 39 milliseconds
        98.18% <= 40 milliseconds
        98.34% <= 41 milliseconds
        98.59% <= 42 milliseconds
        98.75% <= 43 milliseconds
        98.85% <= 44 milliseconds
        98.96% <= 45 milliseconds
        99.09% <= 46 milliseconds
        99.17% <= 47 milliseconds
        99.26% <= 48 milliseconds
        99.34% <= 49 milliseconds
        99.40% <= 50 milliseconds
        99.46% <= 51 milliseconds
        99.52% <= 52 milliseconds
        99.55% <= 53 milliseconds
        99.58% <= 54 milliseconds
        99.62% <= 55 milliseconds
        99.67% <= 56 milliseconds
        99.69% <= 57 milliseconds
        99.71% <= 58 milliseconds
        99.74% <= 59 milliseconds
        99.75% <= 60 milliseconds
        99.76% <= 61 milliseconds
        99.77% <= 62 milliseconds
        99.79% <= 64 milliseconds
        99.81% <= 65 milliseconds
        99.82% <= 66 milliseconds
        99.83% <= 67 milliseconds
        99.84% <= 69 milliseconds
        99.85% <= 70 milliseconds
        99.86% <= 71 milliseconds
        99.87% <= 72 milliseconds
        99.89% <= 74 milliseconds
        99.90% <= 76 milliseconds
        99.92% <= 80 milliseconds
        99.94% <= 83 milliseconds
        99.95% <= 84 milliseconds
        99.96% <= 87 milliseconds
        99.98% <= 90 milliseconds
        99.99% <= 92 milliseconds
        100.00% <= 95 milliseconds
        15974.44 requests per second
    
 
## TODO

The project is woking on the following feature:

- High performance and asynchronous

- Support more redis-like command

- More test cases(both unit test and integrated test)

- Key sharding

- Add more log
