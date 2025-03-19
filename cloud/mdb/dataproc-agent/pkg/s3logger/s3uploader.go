package s3logger

import (
	"bytes"
	"context"
	"fmt"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/request"
	s3session "github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/s3"
	"github.com/aws/aws-sdk-go/service/s3/s3manager"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
)

// s3Uploader implements ability to upload log chunks to Yandex S3.
type s3Uploader struct {
	session         *s3session.Session
	client          *s3.S3
	bucket          string
	filenamePattern string
	isDisabled      bool
}

// newS3Uploader builds s3Uploader object.
func newS3Uploader(config Config, getToken models.GetToken, filenamePattern string) (*s3Uploader, error) {
	awsConfig := &aws.Config{
		Endpoint:         aws.String(config.Endpoint),
		Region:           aws.String(config.RegionName),
		S3ForcePathStyle: aws.Bool(true),
	}
	session, err := s3session.NewSession(awsConfig)
	if err != nil {
		return nil, err
	}

	uploader := &s3Uploader{
		session:         session,
		client:          s3.New(session),
		bucket:          config.BucketName,
		filenamePattern: filenamePattern,
	}
	uploader.applyToken(getToken)
	return uploader, nil
}

// applyToken removes AWS signing algorithm and applies new one that adds
// header containing IAM token.
func (uploader *s3Uploader) applyToken(getToken models.GetToken) {
	uploader.client.Handlers.Sign.Clear()
	handler := request.NamedHandler{
		Name: "yandex-signer",
		Fn: func(req *request.Request) {
			token := getToken()
			req.HTTPRequest.Header.Set("X-YaCloud-SubjectToken", token)
		},
	}
	uploader.client.Handlers.Sign.PushBackNamed(handler)
}

// UploadChunk uploads chunk to S3.
func (uploader *s3Uploader) UploadChunk(ctx context.Context, index int, content []byte) error {
	if uploader.bucket == "" || uploader.isDisabled {
		return nil
	}

	filename := fmt.Sprintf(uploader.filenamePattern, index)

	u := s3manager.NewUploader(uploader.session, func(u *s3manager.Uploader) {
		u.S3 = uploader.client
	})
	_, err := u.UploadWithContext(ctx, &s3manager.UploadInput{
		Bucket: aws.String(uploader.bucket),
		Key:    aws.String(filename),
		Body:   bytes.NewReader(content),
	})
	return err
}

func (uploader *s3Uploader) Disable() {
	uploader.isDisabled = true
}
