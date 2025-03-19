package persistence

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"io/ioutil"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/awserr"
	aws_credentials "github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/session"
	aws_s3 "github.com/aws/aws-sdk-go/service/s3"

	persistence_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence/config"
	task_errors "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type S3Client struct {
	s3 *aws_s3.S3
}

func NewS3Client(
	endpoint string,
	region string,
	credentials S3Credentials,
) (*S3Client, error) {

	sessionConfig := &aws.Config{
		Credentials: aws_credentials.NewStaticCredentials(
			credentials.ID,
			credentials.Secret,
			"", // token - only required for temporary security credentials retrieved via STS, we don't need that
		),
		Endpoint:         &endpoint,
		Region:           &region,
		S3ForcePathStyle: aws.Bool(true), // Without it we get DNS DDOS errors in tests. This option is fine for production too.
	}

	session, err := session.NewSession(sessionConfig)
	if err != nil {
		return nil, task_errors.MakeRetriable(err, false)
	}

	return &S3Client{
		s3: aws_s3.New(session),
	}, nil
}

func NewS3ClientFromConfig(
	config *persistence_config.S3Config,
) (*S3Client, error) {

	credentials, err := NewS3CredentialsFromFile(config.GetCredentialsFilePath())
	if err != nil {
		return nil, err
	}

	return NewS3Client(config.GetEndpoint(), config.GetRegion(), credentials)
}

////////////////////////////////////////////////////////////////////////////////

func (c *S3Client) CreateBucket(
	ctx context.Context,
	bucket string,
) error {

	_, err := c.s3.CreateBucketWithContext(ctx, &aws_s3.CreateBucketInput{
		Bucket: &bucket,
	})
	if err != nil {
		if aerr, ok := err.(awserr.Error); ok {
			switch aerr.Code() {
			case aws_s3.ErrCodeBucketAlreadyOwnedByYou:
				// Bucket is already created
				return nil
			}
		}

		return task_errors.MakeRetriable(err, false)
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func (c *S3Client) GetObject(
	ctx context.Context,
	bucket string,
	key string,
) ([]byte, error) {

	res, err := c.s3.GetObjectWithContext(ctx, &aws_s3.GetObjectInput{
		Bucket: &bucket,
		Key:    &key,
	})
	if err != nil {
		if aerr, ok := err.(awserr.Error); ok {
			switch aerr.Code() {
			case aws_s3.ErrCodeNoSuchKey:
				return nil, &task_errors.NonRetriableError{
					Err: errors.New("chunk not found"),
				}
			case aws_s3.ErrCodeNoSuchBucket:
				return nil, &task_errors.NonRetriableError{Err: err}
			}
		}

		return nil, task_errors.MakeRetriable(err, false)
	}

	objBytes, err := ioutil.ReadAll(res.Body)
	if err != nil {
		return nil, &task_errors.NonRetriableError{Err: err}
	}

	return objBytes, nil
}

func (c *S3Client) PutObject(
	ctx context.Context,
	bucket string,
	key string,
	objBytes []byte,
) error {

	_, err := c.s3.PutObjectWithContext(ctx, &aws_s3.PutObjectInput{
		Bucket: &bucket,
		Key:    &key,
		Body:   bytes.NewReader(objBytes),
	})
	if err != nil {
		if aerr, ok := err.(awserr.Error); ok {
			switch aerr.Code() {
			case aws_s3.ErrCodeNoSuchBucket:
				return &task_errors.NonRetriableError{Err: err}
			}
		}

		return task_errors.MakeRetriable(err, false)
	}

	return nil
}

func (c *S3Client) DeleteObject(
	ctx context.Context,
	bucket string,
	key string,
) error {

	_, err := c.s3.DeleteObjectWithContext(ctx, &aws_s3.DeleteObjectInput{
		Bucket: &bucket,
		Key:    &key,
	})
	if err != nil {
		if aerr, ok := err.(awserr.Error); ok {
			switch aerr.Code() {
			case aws_s3.ErrCodeNoSuchBucket:
				return &task_errors.NonRetriableError{Err: err}
			}
		}

		return task_errors.MakeRetriable(err, false)
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

type S3Credentials struct {
	ID     string `json:"id,omitempty"`
	Secret string `json:"secret,omitempty"`
}

func NewS3Credentials(id, secret string) S3Credentials {
	return S3Credentials{
		ID:     id,
		Secret: secret,
	}
}

func NewS3CredentialsFromFile(filePath string) (S3Credentials, error) {
	file, err := ioutil.ReadFile(filePath)
	if err != nil {
		return S3Credentials{}, err
	}

	credentials := S3Credentials{}

	err = json.Unmarshal(file, &credentials)
	if err != nil {
		return S3Credentials{}, err
	}

	return credentials, nil
}
