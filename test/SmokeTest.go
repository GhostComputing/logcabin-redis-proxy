package main

import (
	"flag"
	"fmt"
	"github.com/go-redis/redis"
	"math/rand"
	"net"
	"os"
	"reflect"
	"strings"
	"time"
)

func init() {
	rand.Seed(time.Now().UnixNano())
}

func isLogCabinRunning(cluster string) bool {
	_, err := net.Dial("tcp", cluster)
	if err != nil {
		return false
	}
	return true
}

func generateRandomKey(prefix string) string {
	return prefix + string(rand.Intn(10000))
}

func testRpushCommand(client *redis.Client) {
	testKey := generateRandomKey("test-rpush")
	testValueList := []string{"a", "b", "c", "d"}

	err := client.RPush(testKey, "a", "b", "c", "d").Err()
	if err != nil {
		if strings.Index(err.Error(), "+OK") != -1 {
			data, err := client.LRange(testKey, 0, -1).Result()
			if err != nil {
				fmt.Printf("=> TEST rpush command failed: %s\n", err.Error())
				os.Exit(1)
			} else if err == redis.Nil {
				fmt.Printf("=> TEST rpush command failed: key is not exist\n")
				os.Exit(1)
			} else {
				if reflect.DeepEqual(data, testValueList) {
					fmt.Printf("=> TEST rpush command successfully\n")
				} else {
					fmt.Printf("=> TEST rpush command failed, expected: %s, get: %s\n", testValueList, data)
					os.Exit(1)
				}
			}
		} else {
			fmt.Printf("=> TEST rpush command failed: %s\n", err.Error())
			os.Exit(1)
		}
	}
}

func testLtrimCommand(client *redis.Client) {
	testKey := generateRandomKey("test-ltrim")

	err := client.RPush(testKey, "one", "two", "three").Err()
	if err != nil {
		if strings.Index(err.Error(), "+OK") != -1 {
			err := client.LTrim(testKey, 1, -1).Err()
			if err != nil {
				fmt.Printf("=> TEST ltrim command failed: %s\n", err.Error())
				os.Exit(1)
			} else {
				data, err := client.LRange(testKey, 0, -1).Result()
				if err != nil {
					fmt.Printf("=> TEST ltrim command failed: %s\n", err.Error())
					os.Exit(1)
				} else if err == redis.Nil {
					fmt.Printf("=> TEST ltrim command failed: key is not exist\n")
					os.Exit(1)
				} else {
					expectedResult := []string{"two", "three"}
					if reflect.DeepEqual(data, expectedResult) {
						fmt.Printf("=> TEST ltrim command successfully\n")
					} else {
						fmt.Printf("=> TEST ltrim command failed, expected: %s, get: %s\n", expectedResult, data)
						os.Exit(1)
					}
				}
			}
		} else {
			fmt.Printf("=> TEST rpush command failed: %s\n", err.Error())
			os.Exit(1)
		}
	}
}

func checkLrangeResult(result []string, expectedResult []string, err error) {
	if err != nil {
		fmt.Printf("=> TEST lrange command failed: %s\n", err.Error())
		os.Exit(1)
	} else {
		if reflect.DeepEqual(result, expectedResult) != true {
			fmt.Printf("=> TEST lrange command failed, expected: %s, get: %s\n", expectedResult, result)
			os.Exit(1)
		}
	}
}

func testLrangeCommand(client *redis.Client) {
	testKey := generateRandomKey("test-lrange")

	err := client.RPush(testKey, "one", "two", "three").Err()
	if err != nil {
		if strings.Index(err.Error(), "+OK") != -1 {
			result1, err1 := client.LRange(testKey, 0, 0).Result()
			result2, err2 := client.LRange(testKey, -3, 2).Result()
			result3, err3 := client.LRange(testKey, -100, 100).Result()
			result4, err4 := client.LRange(testKey, 5, 10).Result()

			checkLrangeResult(result1, []string{"one"}, err1)
			checkLrangeResult(result2, []string{"one", "two", "three"}, err2)
			checkLrangeResult(result3, []string{"one", "two", "three"}, err3)
			checkLrangeResult(result4, []string{}, err4)
			fmt.Printf("=> TEST lrange command successfully\n")
		}
	} else {
		fmt.Printf("=> TEST lrange command failed, %s\n", err.Error())
		os.Exit(1)
	}
}

func testExpireCommand(client *redis.Client) {
	testKey := generateRandomKey("test-expire")

	err := client.RPush(testKey, "one", "two", "three").Err()
	if err != nil {
		err := client.Expire(testKey, 1*time.Second).Err() // TTL is 5 seconds
		time.Sleep(3 * time.Second)
		if err != nil {
			fmt.Printf("=> TEST expire command failed: %s\n", err.Error())
			os.Exit(1)
		} else {
			data, err := client.LRange(testKey, 0, -1).Result()
			if err == nil {
				if reflect.DeepEqual(data, []string{}) == true {
					fmt.Printf("=> TEST expire command successfully\n")
				} else {
					fmt.Printf("=> TEST expire command failed\n")
					os.Exit(1)
				}
			} else {
				fmt.Printf("=> TEST expire command failed, %s\n", err.Error())
				os.Exit(1)
			}
		}
	} else {
		fmt.Printf("=> TEST expire command failed, %s\n", err.Error())
		os.Exit(1)
	}
}

func main() {

	clusterAddr := flag.String("cluster", "localhost:5254", "address of logcabin cluster")
	proxyAddr := flag.String("proxy", "localhost:6380", "address of logcabin redis proxy")

	if len(os.Args) != 3 {
		flag.Usage()
	} else {

		flag.Parse()

		if isLogCabinRunning(*clusterAddr) {
			client := redis.NewClient(&redis.Options{
				Addr:     *proxyAddr,
				Password: "", // no password set
				DB:       0,  // use default DB
			})

			testRpushCommand(client)
			testLtrimCommand(client)
			testLrangeCommand(client)
			testExpireCommand(client)
			os.Exit(0)
		} else {
			fmt.Printf("CRITICAL: logcabin is not running !\n")
			os.Exit(1)
		}
	}
}
