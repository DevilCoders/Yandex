package s3driver

import (
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/s3"

	liblog "a.yandex-team.ru/library/go/core/log"
)

var log liblog.Logger

type S3API interface {
	HeadObject(input *s3.HeadObjectInput) (*s3.HeadObjectOutput, error)
	GetObject(input *s3.GetObjectInput) (*s3.GetObjectOutput, error)
	ListObjectsV2(input *s3.ListObjectsV2Input) (*s3.ListObjectsV2Output, error)
	PutObject(input *s3.PutObjectInput) (*s3.PutObjectOutput, error)
	CreateMultipartUpload(input *s3.CreateMultipartUploadInput) (*s3.CreateMultipartUploadOutput, error)
	UploadPart(input *s3.UploadPartInput) (*s3.UploadPartOutput, error)
	AbortMultipartUpload(input *s3.AbortMultipartUploadInput) (*s3.AbortMultipartUploadOutput, error)
	CompleteMultipartUpload(input *s3.CompleteMultipartUploadInput) (*s3.CompleteMultipartUploadOutput, error)
	DeleteObject(input *s3.DeleteObjectInput) (*s3.DeleteObjectOutput, error)
	UploadPartCopy(input *s3.UploadPartCopyInput) (*s3.UploadPartCopyOutput, error)
}

type S3Driver struct {
	Service S3API
	Prefix  string
}

// NewS3Service initialize s3 service
func NewS3Service(logger liblog.Logger, debug bool) S3API {
	log = logger
	s3config := aws.NewConfig()
	log.Infof("Using '%s/%s' as S3 endpoint and bucket for storage", *Flags.Endpoint, *Flags.Bucket)
	s3config = s3config.
		WithEndpoint(*Flags.Endpoint).
		WithS3ForcePathStyle(true).
		WithLogger(aws.LoggerFunc(func(args ...interface{}) {
			log.Infof("%v", args)
		}))

	if debug {
		s3config = s3config.WithLogLevel(aws.LogDebug)
	}
	return s3.New(session.Must(session.NewSession()), s3config)
}
