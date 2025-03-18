package awstvm

import "github.com/aws/aws-sdk-go/aws/credentials"

var _ credentials.Provider = (*s3AuthProvider)(nil)

type s3AuthProvider struct {
	provider *AuthProvider
}

func (a *s3AuthProvider) Retrieve() (credentials.Value, error) {
	creds, err := a.provider.Retrieve()
	if err != nil {
		return creds, err
	}

	// SessionToken must be prefixed with "TVM2": https://wiki.yandex-team.ru/mds/s3-api/authorization/#avtorizacijapotvm2
	creds.SessionToken = "TVM2 " + creds.SessionToken
	return creds, nil
}

func (a *s3AuthProvider) IsExpired() bool {
	return a.provider.IsExpired()
}
