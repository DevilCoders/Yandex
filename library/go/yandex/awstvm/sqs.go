package awstvm

import (
	"github.com/aws/aws-sdk-go/aws/credentials"

	"a.yandex-team.ru/library/go/yandex/tvm"
)

const (
	SqsTvmID tvm.ClientID = 2002456
)

// NewSqsCredentials returns new aws credential for SQS
// Please read the documentation about TVM usage with SQS: https://wiki.yandex-team.ru/sqs/security
func NewSqsCredentials(tvmClient tvm.Client, account string, opts ...Option) (*credentials.Credentials, error) {
	defaultOpts := []Option{
		WithAccessKeyID(account),
		WithTvmClientID(SqsTvmID),
	}
	authProvider, err := NewAuthProvider(tvmClient, append(defaultOpts, opts...)...)
	if err != nil {
		return nil, err
	}

	return credentials.NewCredentials(authProvider), nil
}
