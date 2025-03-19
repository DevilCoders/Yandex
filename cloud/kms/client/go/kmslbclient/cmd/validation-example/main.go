package main

import (
	"context"
	"crypto/hmac"
	"crypto/sha1"
	"encoding/base64"
	"flag"
	"log"
	"time"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1"
	"a.yandex-team.ru/cloud/kms/client/go/kmslbclient"
)

var (
	controlAddr = flag.String("control-addr", "", "control plane endpoint")
	iamToken    = flag.String("token", "", "IAM token")
	accessKeyID = flag.String("access-key", "", "Key id part of access key")
	secretKey   = flag.String("secret-key", "", "Secret part of access key")
	keyID       = flag.String("key-id", "", "key ID to validate")
)

func prepareSignatureV2(accessKeyID string, secretKey string) *kmslbclient.AccessKeySignatureV2 {
	canonicalRequest := "GET\nexample.com\n/\n"
	signature := hmacSHA1([]byte(secretKey), []byte(canonicalRequest))

	ret := &kmslbclient.AccessKeySignatureV2{}
	ret.KeyID = accessKeyID
	ret.StringToSign = canonicalRequest
	ret.Signature = base64.StdEncoding.EncodeToString(signature)
	ret.SignatureMethod = servicecontrol.AccessKeySignature_Version2Parameters_HMAC_SHA1
	return ret
}

func hmacSHA1(key []byte, data []byte) []byte {
	hash := hmac.New(sha1.New, key)
	hash.Write(data)
	return hash.Sum(nil)
}

func main() {
	flag.Parse()

	if *controlAddr == "" {
		flag.Usage()
		log.Fatal("control-addr is empty")
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

	startTime := time.Now()

	validator, err := kmslbclient.NewKeyValidator(*controlAddr)
	if err != nil {
		log.Fatalf("failed creating key validator: %v", err)
	}
	var callCreds *kmslbclient.CallCredentials
	if *accessKeyID != "" {
		callCreds = &kmslbclient.CallCredentials{SignatureV2: prepareSignatureV2(*accessKeyID, *secretKey)}
	} else {
		callCreds = &kmslbclient.CallCredentials{IAMToken: iamToken}
	}
	isValid, err := validator.ValidateKey(context.Background(), *keyID, callCreds)
	if err != nil {
		log.Fatalf("failed to check validity: %v", err)
	}
	log.Printf("Checked validity of %s: %t in %dms", *keyID, isValid, time.Since(startTime).Milliseconds())
}
