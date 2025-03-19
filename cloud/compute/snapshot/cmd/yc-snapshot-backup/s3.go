package main

import (
	"bytes"
	"context"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/s3"
)

const s3Endpoint = "https://s3.mds.yandex.net"

var s3Client *s3.S3

func init() {
	sess := session.Must(session.NewSession(&aws.Config{
		Endpoint:    aws.String(s3Endpoint),
		Credentials: credentials.NewStaticCredentials(keyID, keySecret, ""),
		Region:      aws.String("ru-central1"),
	}))
	s3Client = s3.New(sess)
}

func s3Put(bucket, key string, content []byte) error {
	ctx, ctxCancel := context.WithTimeout(context.Background(), time.Second*30)
	defer ctxCancel()

	_, err := s3Client.PutObjectWithContext(ctx, &s3.PutObjectInput{
		Bucket: aws.String(bucket),
		Key:    aws.String(key),
		Body:   bytes.NewReader(content),
	},
	)
	return err
}
