package s3

import (
	"context"
	"io"
	"time"

	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../scripts/mockgen.sh Client

const MaxKeysLimit = 1000

type Bucket struct {
	CreationDate time.Time
	Name         string
}

type ListObjectsOpts struct {
	// Sets the maximum number of keys returned in the response. The response might
	// contain fewer keys but will never contain more.
	MaxKeys *int64
	// Limits the response to keys that begin with the specified prefix.
	Prefix *string
	// A delimiter is a character you use to group keys.
	Delimiter *string
}

func (o ListObjectsOpts) Validate() error {
	if o.MaxKeys != nil && *o.MaxKeys > MaxKeysLimit {
		return ErrMaxKeysReached
	}
	return nil
}

type Object struct {
	Key          string
	LastModified time.Time
	Size         int64
}

type Prefix struct {
	Prefix string
}

type PutObjectOpts struct {
	ContentLength *int64
	ContentType   *string
}

var ErrNotFound = xerrors.NewSentinel("not found")
var ErrMaxKeysReached = xerrors.NewSentinel("max keys limit reached")

type Client interface {
	ListBuckets(ctx context.Context) ([]Bucket, error)
	ListObjects(ctx context.Context, bucket string, opts ListObjectsOpts) ([]Object, []Prefix, error)
	CopyObject(ctx context.Context, srcBucket, srcKey, dstBucket, dstKey string) error
	GetObject(ctx context.Context, bucket, key string) (io.ReadCloser, error)
	PutObject(ctx context.Context, bucket, key string, body io.ReadSeeker, opts PutObjectOpts) error
	DeleteObject(ctx context.Context, bucket, key string) error
}
