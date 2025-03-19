package main

import (
	"bytes"
	"context"
	"crypto/hmac"
	"crypto/sha1"
	"crypto/sha256"
	"encoding/base64"
	"encoding/hex"
	"flag"
	"log"
	"sort"
	"strings"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1"
	"a.yandex-team.ru/cloud/kms/client/go/kmslbclient"
	alog "a.yandex-team.ru/library/go/core/log"
)

var (
	discoveryAddr = flag.String("discovery", "", "discovery endpoint")
	addrs         = flag.String("addrs", "", "comma-separated list of endpoints to connect to")
	iamToken      = flag.String("token", "", "IAM token")
	accessKeyID   = flag.String("access-key", "", "Key id part of access key")
	secretKey     = flag.String("secret-key", "", "Secret part of access key")
	tlsServerName = flag.String("tls-server-name", "", "replace hostname in TLS validation (useful for directly connecting to backends)")
	keyID         = flag.String("key-id", "", "key ID to use for encryption/decryption")
	numRequests   = flag.Int("requests", 100, "number of encrypt/decrypt requests to do per thread")
	numThreads    = flag.Int("threads", 1, "number of threads submitting encrypt/decrypt requests")
	dataSize      = flag.Int("data-size", 32, "size of plaintext")
	logLevel      = flag.String("log-level", "info", "additional logging for the KMS client")
	maxConcurrent = flag.Int("max-concurrent", 0, "maximum number of concurrent requests to instance")
	maxTimePerTry = flag.Int("max-time-per-try", 1, "maximum time to wait before retrying the request in seconds")
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

type accessKey struct {
	accessKeyID string
	secretKey   string
}

func workerMain(client kmslbclient.KMSClient, workerNum int, ak accessKey) workerResult {
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

		var callCreds *kmslbclient.CallCredentials
		if ak.accessKeyID != "" {
			if i%2 == 0 {
				callCreds = &kmslbclient.CallCredentials{
					SignatureV2: prepareSignatureV2(ak),
				}
			} else {
				callCreds = &kmslbclient.CallCredentials{
					SignatureV4: prepareSignatureV4(ak),
				}
			}
		}

		if i%2 == 0 {
			// Use different plaintext for each request.
			plaintext[0] = byte(i)

			start := time.Now()
			ciphertext, err = client.Encrypt(context.Background(), *keyID, plaintext, aad, callCreds)
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
			newPlaintext, err = client.Decrypt(context.Background(), *keyID, ciphertext, aad, callCreds)
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

func prepareSignatureV2(ak accessKey) *kmslbclient.AccessKeySignatureV2 {
	canonicalRequest := "GET\nexample.com\n/\n"
	signature := hmacSHA1([]byte(ak.secretKey), []byte(canonicalRequest))

	ret := &kmslbclient.AccessKeySignatureV2{}
	ret.KeyID = ak.accessKeyID
	ret.StringToSign = canonicalRequest
	ret.Signature = base64.StdEncoding.EncodeToString(signature)
	ret.SignatureMethod = servicecontrol.AccessKeySignature_Version2Parameters_HMAC_SHA1
	return ret
}

func prepareSignatureV4(ak accessKey) *kmslbclient.AccessKeySignatureV4 {
	region := "us-east-1"
	service := "s3"

	signTime := time.Now()
	timeString := signTime.Format("20060102T150405Z")
	scopeString := signTime.Format("20060102") + "/" + region + "/" + service + "/aws4_request"
	canonicalRequestHash := makeEmptyCanonicalRequestHashV4()
	stringToSign := "AWS4-HMAC-SHA256\n" + timeString + "\n" + scopeString + "\n" + canonicalRequestHash

	derivedKey := deriveKeyV4(ak.secretKey, region, service, signTime)
	signature := hmacSHA256(derivedKey, []byte(stringToSign))

	ret := &kmslbclient.AccessKeySignatureV4{}
	ret.KeyID = ak.accessKeyID
	ret.StringToSign = stringToSign
	ret.Signature = hex.EncodeToString(signature)
	ret.Region = region
	ret.SignedAt = signTime
	ret.Service = service
	return ret
}

func makeEmptyCanonicalRequestHashV4() string {
	emptyStringSHA256 := "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
	canonicalRequest := "GET\n/\n\n\n\n\n" + emptyStringSHA256
	h := sha256.New()
	h.Write([]byte(canonicalRequest))
	return hex.EncodeToString(h.Sum(nil))
}

func deriveKeyV4(secretKey string, region string, service string, signTime time.Time) []byte {
	trimmedKey := strings.TrimSpace(secretKey)
	keyDate := hmacSHA256([]byte("AWS4"+trimmedKey), []byte(signTime.Format("20060102")))
	keyRegion := hmacSHA256(keyDate, []byte(region))
	keyService := hmacSHA256(keyRegion, []byte(service))
	return hmacSHA256(keyService, []byte("aws4_request"))
}

func hmacSHA1(key []byte, data []byte) []byte {
	hash := hmac.New(sha1.New, key)
	hash.Write(data)
	return hash.Sum(nil)
}

func hmacSHA256(key []byte, data []byte) []byte {
	hash := hmac.New(sha256.New, key)
	hash.Write(data)
	return hash.Sum(nil)
}

func main() {
	flag.Parse()

	if *addrs == "" && *discoveryAddr == "" {
		flag.Usage()
		log.Fatal("both addrs and discovery-addr is empty")
	} else if *addrs != "" && *discoveryAddr != "" {
		flag.Usage()
		log.Fatal("both addrs and discovery-addr are not empty")
	}
	if *iamToken == "" && *accessKeyID == "" {
		flag.Usage()
		log.Fatal("both token and access-key are empty")
	}
	if *iamToken != "" && *accessKeyID != "" {
		flag.Usage()
		log.Fatal("both token and access-key are set")
	}
	if (*accessKeyID != "" && *secretKey == "") || (*accessKeyID == "" && *secretKey != "") {
		flag.Usage()
		log.Fatal("both access-key and secret-key should be set")
	}
	if *keyID == "" {
		flag.Usage()
		log.Fatal("key-id is empty")
	}

	clientAddr := &kmslbclient.ClientAddress{}
	if *discoveryAddr != "" {
		clientAddr.DiscoveryAddr = *discoveryAddr
	} else {
		clientAddr.TargetAddrs = strings.Split(*addrs, ",")
	}
	tokenProvider := func() string { return *iamToken }
	logLevel, err := alog.ParseLevel(*logLevel)
	if err != nil {
		log.Fatal(err)
	}
	logger := kmslbclient.NewLoggerFromLog(log.New(log.Writer(), "KMS", log.LstdFlags), logLevel)
	options := &kmslbclient.ClientOptions{
		TokenProvider: tokenProvider,
		TLSTarget:     *tlsServerName,
		Logger:        logger,
		Balancer: kmslbclient.BalancerOptions{
			MaxConcurrentRequestsPerHost: *maxConcurrent,
			MaxTimePerTry:                time.Second * time.Duration(*maxTimePerTry),
		},
	}

	client, err := kmslbclient.NewLBSymmetricCryptoClient(clientAddr, options)
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
			results[threadNum] = workerMain(client, threadNum, accessKey{accessKeyID: *accessKeyID, secretKey: *secretKey})
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
