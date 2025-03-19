package grpc

import (
	"context"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
)

const (
	authHeader  = "Authorization"
	tokenPrefix = "Bearer "
)

type TokenFunc func(ctx context.Context) (iam.Token, error)

func (c *TokenServiceClient) ServiceAccountCredentials(serviceAccount iam.ServiceAccount) iam.CredentialsService {
	return NewTokenCredentials(func(ctx context.Context) (iam.Token, error) {
		return c.ServiceAccountToken(ctx, serviceAccount)
	})
}

func (c *TokenServiceClient) FromOauthCredentials(oauth string) iam.CredentialsService {
	return NewTokenCredentials(func(ctx context.Context) (iam.Token, error) {
		return c.TokenFromOauth(ctx, oauth)
	})
}

func NewTokenCredentials(tf TokenFunc) iam.CredentialsService {
	return &iamTokenCredentialsCached{
		token: newCachedToken(tf),
	}
}

var _ iam.CredentialsService = &iamTokenCredentialsCached{}

type iamTokenCredentialsCached struct {
	token *cachedToken
}

func (i *iamTokenCredentialsCached) Token(ctx context.Context) (string, error) {
	return i.token.Token(ctx)
}

func (i *iamTokenCredentialsCached) GetRequestMetadata(ctx context.Context, uri ...string) (map[string]string, error) {
	token, err := i.token.Token(ctx)
	if err != nil {
		return nil, err
	}
	return map[string]string{authHeader: tokenPrefix + token}, nil
}

func (i *iamTokenCredentialsCached) RequireTransportSecurity() bool {
	return true
}

// expiryDelta determines how earlier a token should be considered
// expired than its actual expiration time. It is used to avoid late
// expirations due to client-server time mismatches.
const expiryDelta = time.Minute

type cachedToken struct {
	tokenF TokenFunc
	nowF   func() time.Time

	mu    sync.Mutex
	token iam.Token
}

func newCachedToken(tokenF TokenFunc) *cachedToken {
	return &cachedToken{tokenF: tokenF, nowF: time.Now}
}

func (ts *cachedToken) Token(ctx context.Context) (string, error) {
	ts.mu.Lock()
	defer ts.mu.Unlock()
	if ts.token.Value != "" && !ts.tokenExpired() {
		return ts.token.Value, nil
	}
	newToken, err := ts.tokenF(ctx)
	if err != nil {
		return "", err
	}
	ts.token = newToken
	return newToken.Value, nil
}

func (ts *cachedToken) tokenExpired() bool {
	return ts.token.ExpiresAt.Sub(ts.nowF()) < expiryDelta
}
