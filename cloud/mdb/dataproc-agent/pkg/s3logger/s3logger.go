// Package s3logger implements log upload to Yandex S3 storage.
package s3logger

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/library/go/core/log"
)

//go:generate ../../../scripts/mockgen.sh ChunkUploader

// Config struct for S3Logger
type Config struct {
	Endpoint      string        `json:"endpoint" yaml:"endpoint"`
	RegionName    string        `json:"region_name" yaml:"region_name"`
	BucketName    string        `json:"bucket_name" yaml:"bucket_name"`
	ChunkSize     int           `json:"chunk_size" yaml:"chunk_size"`
	MaxPoolSize   int           `json:"max_pool_size" yaml:"max_pool_size"`
	SyncInterval  time.Duration `json:"sync_interval" yaml:"sync_interval"`
	UploadTimeout time.Duration `json:"upload_timeout" yaml:"upload_timeout"`
	FlushTimeout  time.Duration `json:"flush_timeout" yaml:"flush_timeout"`
}

// DefaultConfig generates default config for S3Logger
func DefaultConfig() Config {
	return Config{
		Endpoint:      "https://storage.yandexcloud.net",
		RegionName:    "ru-central1",
		BucketName:    "missing-bucket",
		ChunkSize:     4 * 1024,
		MaxPoolSize:   1024 * 1024,
		SyncInterval:  time.Second,
		UploadTimeout: time.Second,
		FlushTimeout:  time.Minute,
	}
}

// New creates PipeProcessor object.
// tokenProvider is responsible for providing valid IAM token.
// filenamePattern is a bucket's path patten to save log chunks to;
// pattern is filled with chunk's serial number.
func New(config Config, getToken models.GetToken, logger log.Logger,
	filenamePattern string) (*PipeProcessor, error) {
	s3uploader, err := newS3Uploader(config, getToken, filenamePattern)
	if err != nil {
		return nil, err
	}

	return newPipeProcessor(s3uploader, config, logger), nil
}
