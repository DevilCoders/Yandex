package main

import (
	"bytes"
	"context"
	"crypto/tls"
	"encoding/base64"
	"encoding/json"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"math"
	"math/rand"
	"net/http"
	"sort"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/backoff"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/keepalive"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/kms/v1"
)

// Mostly copied from cloud/kms/client/go/kmslbclient/cmd/example

const (
	jitter = 0.25
)

var (
	addr                = flag.String("addr", "", "endpoint to connect to")
	iamToken            = flag.String("token", "", "IAM token")
	tlsServerName       = flag.String("tls-server-name", "", "replace hostname in TLS validation (useful for directly connecting to backends)")
	useGrpc             = flag.Bool("use-grpc", false, "use GRPC instead of HTTP")
	keyID               = flag.String("key-id", "", "key ID to use for encryption/decryption")
	numRequests         = flag.Int("requests", 100, "number of encrypt/decrypt requests to do per thread")
	numThreads          = flag.Int("threads", 1, "number of threads submitting encrypt/decrypt requests")
	numRetries          = flag.Int("retries", 3, "number of retries for HTTP 50x or GRPC UNAVAILABLE errors")
	minRetryBackoffMsec = flag.Int("min-backoff", 10, "duration of first backoff when retrying")
	backoffMultiplier   = flag.Float64("backoff-multiplier", 1.5, "multiply backoff on each retry")
	timeoutSec          = flag.Int("timeout", 5, "timeouts for everything, in seconds")
	dataSize            = flag.Int("data-size", 32, "size of plaintext")
)

type workerResult struct {
	encryptP50  time.Duration
	encryptP95  time.Duration
	encryptP99  time.Duration
	encryptP100 time.Duration

	decryptP50  time.Duration
	decryptP95  time.Duration
	decryptP99  time.Duration
	decryptP100 time.Duration

	errors int
}

type Client struct {
	httpClient http.Client
	httpHost   string

	grpcConns    []*grpc.ClientConn
	nextGrpcConn int32
}

type iamTokenFlagAuth struct {
}

func (t iamTokenFlagAuth) GetRequestMetadata(_ context.Context, _ ...string) (map[string]string, error) {
	return map[string]string{
		"Authorization": "Bearer " + *iamToken,
	}, nil
}

func (iamTokenFlagAuth) RequireTransportSecurity() bool {
	return false
}

func makeClient() (*Client, error) {
	timeout := time.Duration(*timeoutSec) * time.Second

	if *useGrpc {
		var dialOptions []grpc.DialOption
		dialOptions = append(dialOptions, grpc.WithTransportCredentials(credentials.NewTLS(&tls.Config{
			ServerName: *tlsServerName,
		})))
		if timeout > 0 {
			dialOptions = append(dialOptions, grpc.WithKeepaliveParams(keepalive.ClientParameters{
				Time:    timeout,
				Timeout: timeout,
			}))
			dialOptions = append(dialOptions, grpc.WithConnectParams(grpc.ConnectParams{
				Backoff:           backoff.DefaultConfig,
				MinConnectTimeout: timeout,
			}))
		}
		dialOptions = append(dialOptions, grpc.WithPerRPCCredentials(iamTokenFlagAuth{}))
		var grpcConns []*grpc.ClientConn
		for i := 0; i < *numThreads; i++ {
			var conn *grpc.ClientConn
			var err error
			if timeout > 0 {
				dialCtx, cancelFunc := context.WithTimeout(context.Background(), timeout)
				defer cancelFunc()
				conn, err = grpc.DialContext(dialCtx, *addr, dialOptions...)
				if err != nil {
					return nil, err
				}
			} else {
				conn, err = grpc.Dial(*addr, dialOptions...)
				if err != nil {
					return nil, err
				}
			}
			grpcConns = append(grpcConns, conn)
		}
		return &Client{
			grpcConns:    grpcConns,
			nextGrpcConn: 0,
		}, nil
	}

	transport := &http.Transport{
		TLSClientConfig:     &tls.Config{ServerName: *tlsServerName},
		MaxIdleConns:        *numThreads,
		MaxIdleConnsPerHost: *numThreads,
		MaxConnsPerHost:     *numThreads,
	}
	httpHost := *addr
	if !strings.HasPrefix(httpHost, "https://") {
		httpHost = "https://" + *addr
	}
	return &Client{
		httpClient: http.Client{Transport: transport, Timeout: timeout},
		httpHost:   httpHost,
	}, nil
}

func sleepRetry(retryNum int) {
	jitterMult := 1.0 - jitter*rand.Float64()
	duration := float64(*minRetryBackoffMsec) * math.Pow(*backoffMultiplier, float64(retryNum)) * jitterMult
	time.Sleep(time.Millisecond * time.Duration(duration))
}

func (c *Client) doHTTPPostJSON(url string, requestMap map[string]string) (map[string]string, error) {
	finalURL := c.httpHost + url
	body, err := json.Marshal(requestMap)
	if err != nil {
		return nil, err
	}
	// trace := &httptrace.ClientTrace{
	// 	ConnectStart: func(network, addr string) {
	// 		log.Printf("ConnectStart(%v, %v)", network, addr)
	// 	},
	// 	TLSHandshakeStart: func() {
	// 		log.Printf("TLSHandshakeStart")
	// 	},
	// 	PutIdleConn: func(err error) {
	// 		log.Printf("PutIdleConn(%v)", err)
	// 	},
	// }
	// request = request.WithContext(httptrace.WithClientTrace(request.Context(), trace))
	for i := 0; i < *numRetries; i++ {
		request, err := http.NewRequest("POST", finalURL, bytes.NewBuffer(body))
		if err != nil {
			return nil, err
		}
		request.Header.Set("Content-Type", "application/json")
		request.Header.Set("Authorization", "Bearer "+*iamToken)
		resp, err := c.httpClient.Do(request)

		lastRetry := i == *numRetries-1
		if err != nil {
			if lastRetry {
				return nil, err
			}
			sleepRetry(i)
			continue
		}
		defer resp.Body.Close()

		respBody, err := ioutil.ReadAll(resp.Body)
		if err != nil {
			if lastRetry {
				return nil, err
			}
			sleepRetry(i)
			continue
		}

		if resp.StatusCode != 200 {
			if lastRetry || resp.StatusCode < 500 {
				return nil, fmt.Errorf("got status %v: %v", resp.Status, string(respBody))
			}
			sleepRetry(i)
			continue
		}

		var result map[string]string
		err = json.Unmarshal(respBody, &result)
		if err != nil {
			return nil, err
		}
		return result, nil
	}
	panic("unreachable")
}

func (c *Client) getNextGrpcClient() kms.SymmetricCryptoServiceClient {
	nextIndex := int(atomic.AddInt32(&c.nextGrpcConn, 1)) % len(c.grpcConns)
	conn := c.grpcConns[nextIndex]
	return kms.NewSymmetricCryptoServiceClient(conn)
}

func doGrpcRetry(call func() (interface{}, error)) (interface{}, error) {
	for i := 0; i < *numRetries; i++ {
		ret, err := call()
		lastRetry := i == *numRetries-1
		if err != nil {
			if lastRetry {
				return nil, err
			}
			if st := status.Code(err); st != codes.Unavailable {
				return nil, err
			}
			sleepRetry(i)
			continue
		}
		return ret, nil
	}
	panic("unreachable")
}

func (c *Client) Encrypt(ctx context.Context, keyID string, plaintext []byte, aad []byte) ([]byte, error) {
	if *useGrpc {
		response, err := doGrpcRetry(func() (interface{}, error) {
			return c.getNextGrpcClient().Encrypt(ctx, &kms.SymmetricEncryptRequest{
				KeyId:      keyID,
				Plaintext:  plaintext,
				AadContext: aad,
			})
		})
		if err != nil {
			return nil, err
		}
		return response.(*kms.SymmetricEncryptResponse).Ciphertext, err
	}

	requestMap := map[string]string{
		"keyId":     keyID,
		"plaintext": base64.StdEncoding.EncodeToString(plaintext),
	}
	if len(aad) > 0 {
		requestMap["aadContext"] = base64.StdEncoding.EncodeToString(aad)
	}
	result, err := c.doHTTPPostJSON("/kms/v1/keys/"+keyID+":encrypt", requestMap)
	if err != nil {
		return nil, err
	}
	ciphertext, err := base64.StdEncoding.DecodeString(result["ciphertext"])
	if err != nil {
		return nil, err
	}
	return ciphertext, nil
}

func (c *Client) Decrypt(ctx context.Context, keyID string, ciphertext []byte, aad []byte) ([]byte, error) {
	if *useGrpc {
		response, err := doGrpcRetry(func() (interface{}, error) {
			return c.getNextGrpcClient().Decrypt(ctx, &kms.SymmetricDecryptRequest{
				KeyId:      keyID,
				Ciphertext: ciphertext,
				AadContext: aad,
			})
		})
		if err != nil {
			return nil, err
		}
		return response.(*kms.SymmetricDecryptResponse).Plaintext, nil
	}

	url := "/kms/v1/keys/" + keyID + ":decrypt"
	requestMap := map[string]string{
		"keyId":      keyID,
		"ciphertext": base64.StdEncoding.EncodeToString(ciphertext),
	}
	if len(aad) > 0 {
		requestMap["aadContext"] = base64.StdEncoding.EncodeToString(aad)
	}
	result, err := c.doHTTPPostJSON(url, requestMap)
	if err != nil {
		return nil, err
	}
	plaintext, err := base64.StdEncoding.DecodeString(result["plaintext"])
	if err != nil {
		return nil, err
	}
	return plaintext, nil
}

func (c *Client) Close() {
	if *useGrpc {
		for _, conn := range c.grpcConns {
			_ = conn.Close()
		}
	}
}

func workerMain(client *Client, workerNum int) workerResult {
	plaintext := make([]byte, *dataSize)
	aad := make([]byte, *dataSize)
	var ciphertext []byte
	var newPlaintext []byte

	for i := 0; i < *dataSize; i++ {
		plaintext[i] = 0x12
	}
	for i := 0; i < *dataSize; i++ {
		aad[i] = byte(workerNum)
	}

	encryptTimes := make([]time.Duration, 0, *numRequests/2)
	decryptTimes := make([]time.Duration, 0, *numRequests/2)

	var err error
	var result workerResult

	for i := 0; i < *numRequests; i++ {
		if i%1000 == 0 {
			log.Printf("Worker #%d request %d\n", workerNum, i)
		}

		if i%2 == 0 {
			// Use different plaintext for each request.
			plaintext[0] = byte(i)

			start := time.Now()
			ciphertext, err = client.Encrypt(context.Background(), *keyID, plaintext, aad)
			delta := time.Since(start)
			if err != nil {
				log.Printf("ERROR: worker #%d failed to encrypt, error: %v\n", workerNum, err)
				result.errors++
				// Skip the next decrypt.
				i++
				continue
			}
			encryptTimes = append(encryptTimes, delta)
		} else {
			start := time.Now()
			newPlaintext, err = client.Decrypt(context.Background(), *keyID, ciphertext, aad)
			delta := time.Since(start)
			if err != nil {
				log.Printf("ERROR: worker #%d failed to decrypt, error: %v\n", workerNum, err)
				result.errors++
				continue
			}
			if !bytes.Equal(plaintext, newPlaintext) {
				log.Fatalf("plaintext and newPlaintext mismatch\n")
			}
			decryptTimes = append(decryptTimes, delta)
		}
	}

	sort.Slice(encryptTimes, func(i int, j int) bool {
		return encryptTimes[i] < encryptTimes[j]
	})
	if len(encryptTimes) > 0 {
		s := len(encryptTimes)
		result.encryptP50 = encryptTimes[s/2]
		result.encryptP95 = encryptTimes[s*95/100]
		result.encryptP99 = encryptTimes[s*99/100]
		result.encryptP100 = encryptTimes[s-1]
	}
	sort.Slice(decryptTimes, func(i int, j int) bool {
		return decryptTimes[i] < decryptTimes[j]
	})
	if len(decryptTimes) > 0 {
		s := len(decryptTimes)
		result.decryptP50 = decryptTimes[s/2]
		result.decryptP95 = decryptTimes[s*95/100]
		result.decryptP99 = decryptTimes[s*99/100]
		result.decryptP100 = decryptTimes[s-1]
	}
	return result
}

func main() {
	flag.Parse()

	if *addr == "" {
		flag.Usage()
		log.Fatal("addr is empty")
	}
	if *iamToken == "" {
		flag.Usage()
		log.Fatal("token is empty")
	}
	if *keyID == "" {
		flag.Usage()
		log.Fatal("keyID is empty")
	}

	client, err := makeClient()
	if err != nil {
		log.Fatalf("error when creating client: %v", err)
	}
	defer client.Close()

	results := make([]workerResult, *numThreads)
	wg := sync.WaitGroup{}
	wg.Add(*numThreads)
	start := time.Now()
	for t := 0; t < *numThreads; t++ {
		go func(threadNum int) {
			results[threadNum] = workerMain(client, threadNum)
			wg.Done()
		}(t)
	}
	wg.Wait()
	delta := time.Since(start)

	totalRequests := int64(*numRequests) * int64(*numThreads)
	rps := int64(totalRequests * 1000 / delta.Milliseconds())
	totalErrors := 0
	for i := 0; i < *numThreads; i++ {
		log.Printf("Times for worker #%d: decrypt p50 %dusec, p95: %dusec, p99: %dusec, p100: %dusec, encrypt p50: %dusec, p95: %dusec, p99: %dusec, p100: %dusec\n",
			i, results[i].decryptP50.Microseconds(), results[i].decryptP95.Microseconds(), results[i].decryptP99.Microseconds(), results[i].decryptP100.Microseconds(),
			results[i].encryptP50.Microseconds(), results[i].encryptP95.Microseconds(), results[i].encryptP99.Microseconds(), results[i].encryptP100.Microseconds())
		totalErrors += results[i].errors
	}

	log.Printf("Success. Total RPS: %d, total errors: %d\n", rps, totalErrors)
}
