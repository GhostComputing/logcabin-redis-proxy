## Overview

logcabin-redis-proxy is the proxy that supports redis compatible protocol and working on LogCabin cluster.

## Getting started

### Compile logcabin-redis-proxy

- make sure your g++ supports C++11;

- run the following command:

      $ git submodule update --init # download submodule simple_resp / logcabin / ThreadPool
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

## Run smoke test

- make sure logcabin cluster and logcabin_redis_proxy is running;

- make sure your golang working environment(the test code depends on [go-redis](https://github.com/go-redis/redis))

- execuate following command(suppose your logcabin cluster and proxy are all running at '192.168.2.109' address):

      $ go run test/SmokeTest.go --cluster=192.168.2.109:5254 --proxy=192.168.2.109:6380
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
        Benchmark took 636.191 ms to write 10000 objects OPS: 15718.5
        0.22% <= 1 milliseconds
        1.28% <= 2 milliseconds
        45.56% <= 3 milliseconds
        95.28% <= 4 milliseconds
        99.70% <= 5 milliseconds
        99.98% <= 6 milliseconds
        100.00% <= 7 milliseconds
        
- Run redis benchmark

        $./logcabin_redis_proxy -c $LOCALSERVER -s 10240 -p 6380 -a $PROXY -t 50
        
        $ redis-benchmark -h $PROXY -p 6380 -t rpush -c 100 -n 10000 -r 100000000
         ====== RPUSH ======
           10000 requests completed in 0.64 seconds
           100 parallel clients
           3 bytes payload
           keep alive: 1
         
         0.01% <= 1 milliseconds
         0.03% <= 2 milliseconds
         0.13% <= 3 milliseconds
         0.19% <= 4 milliseconds
         0.71% <= 5 milliseconds
         41.99% <= 6 milliseconds
         83.02% <= 7 milliseconds
         91.51% <= 8 milliseconds
         95.96% <= 9 milliseconds
         98.58% <= 10 milliseconds
         99.73% <= 11 milliseconds
         99.98% <= 12 milliseconds
         100.00% <= 12 milliseconds
         15552.10 requests per second
 
## Supported commands

- `RPUSH`
   
   - **Caveat**: it just return "+OK" instead of the key size (because 
     logcabin cluster don't support redis-like return value yet);
   
- `LRANGE`

- `LTRIM`

- `EXPIRE`
