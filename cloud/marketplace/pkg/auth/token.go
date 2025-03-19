package auth

import (
	"context"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"

	ydbiam "a.yandex-team.ru/kikimr/public/sdk/go/ydb/auth/iam"
)

type DefaultTokenAuth interface {
	Token(ctx context.Context) (string, error)
}

type YCDefaultTokenAuthenticator struct {
	creds ydb.Credentials
}

func NewYCDefaultTokenAuthenticator(ctx context.Context) *YCDefaultTokenAuthenticator {
	return &YCDefaultTokenAuthenticator{
		creds: ydbiam.InstanceServiceAccount(ctx),
	}
}

func NewYCDefaultTokenAuthenticatorWithURL(ctx context.Context, metadataURL string) *YCDefaultTokenAuthenticator {
	return &YCDefaultTokenAuthenticator{
		creds: ydbiam.InstanceServiceAccountURL(ctx, metadataURL),
	}
}

func (a *YCDefaultTokenAuthenticator) Token(ctx context.Context) (string, error) {
	return a.creds.Token(ctx)
}

type TokenAuthenticator string

func NewTokenAuthenticator(token string) TokenAuthenticator {
	return TokenAuthenticator(token)
}

func (t TokenAuthenticator) Token(ctx context.Context) (string, error) {
	return string(t), nil
}
