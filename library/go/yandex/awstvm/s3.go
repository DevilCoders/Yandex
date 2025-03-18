package awstvm

import (
	"github.com/aws/aws-sdk-go/aws/credentials"

	"a.yandex-team.ru/library/go/yandex/tvm"
)

const (
	S3TvmID        tvm.ClientID = 2017579
	S3TestingTvmID tvm.ClientID = 2017577
)

// NewS3Credentials returns new aws credential for S3
// Please read the documentation about TVM usage with S3: https://wiki.yandex-team.ru/mds/s3-api/authorization/#avtorizacijapotvm2
func NewS3Credentials(tvmClient tvm.Client, opts ...Option) (*credentials.Credentials, error) {
	defaultOpts := []Option{
		WithTvmClientID(S3TvmID),
	}

	authProvider, err := NewAuthProvider(tvmClient, append(defaultOpts, opts...)...)
	if err != nil {
		return nil, err
	}

	return credentials.NewCredentials(&s3AuthProvider{
		provider: authProvider,
	}), nil
}
