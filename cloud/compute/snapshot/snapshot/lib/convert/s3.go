package convert

import (
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/directreader"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/awserr"
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/s3"
	"go.uber.org/zap"
	"golang.org/x/net/context"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/nbd"
)

var (
	timeout = 24 * time.Hour
)

// S3source represents an S3-compatible image source.
// Deprecated.
type S3source struct {
	*s3.S3
	EnableRedirects bool
	proxySock       string
}

// NewS3 returns a new S3source instance.
// Deprecated.
func NewS3(conf *config.S3Config, proxySock string) S3source {
	// NOTE: disable for now, as MDS has no auth mechanism
	// sess := session.Must(session.NewSessionWithOptions(session.Options{Profile: conf.Profile}))
	// p := &ec2rolecreds.EC2RoleProvider{
	// 	Client: ec2metadata.New(sess,
	// 		aws.NewConfig().WithEndpoint(conf.TokenEndpoint)),
	// }

	s3sess := session.Must(session.NewSessionWithOptions(session.Options{
		Config:  aws.Config{Credentials: credentials.AnonymousCredentials},
		Profile: conf.Profile,
	}))
	return S3source{
		S3:              s3.New(s3sess, aws.NewConfig().WithRegion(conf.RegionName).WithEndpoint(conf.Endpoint)),
		EnableRedirects: conf.EnableRedirects,
		proxySock:       proxySock,
	}
}

// Get returns an image from S3 object.
func (s3c S3source) Get(ctx context.Context, req *common.ConvertRequest, blockMap directreader.BlocksMap) (nbd.Image, error) {
	if req.Bucket == "" || req.Key == "" {
		return nil, misc.ErrUnknownSource
	}

	ctx = log.WithLogger(ctx, log.G(ctx).With(zap.String("bucket", req.Bucket), zap.String("key", req.Key)))

	var h *s3.HeadObjectOutput
	err := misc.Retry(ctx, "s3.HeadObject", func() error {
		var err error
		h, err = s3c.HeadObject((&s3.HeadObjectInput{}).SetBucket(req.Bucket).SetKey(req.Key))
		switch err := err.(type) {
		case nil:
			return nil
		case awserr.Error:
			if err.Code() == s3.ErrCodeNoSuchKey {
				log.G(ctx).Error("s3.HeadObject failed", zap.Error(err))
				return misc.ErrInvalidObject
			}
		}
		log.G(ctx).Error("s3.HeadObject failed", zap.Error(err))
		return misc.ErrInternalRetry
	})
	if err != nil {
		return nil, err
	}

	if h.ContentLength == nil || *h.ContentLength == 0 {
		log.G(ctx).Error("Object has zero length")
		return nil, misc.ErrInvalidObject
	}

	r, _ := s3c.GetObjectRequest((&s3.GetObjectInput{}).SetBucket(req.Bucket).SetKey(req.Key))
	url, err := r.Presign(timeout)
	if err != nil {
		log.G(ctx).Error("Cannot get presigned url", zap.Error(err))
		return nil, err
	}

	return newHTTPImage(ctx, url, req.Format, s3c.EnableRedirects, s3c.proxySock, blockMap)
}
