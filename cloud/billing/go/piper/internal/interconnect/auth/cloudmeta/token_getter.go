package cloudmeta

import (
	"context"
	"time"

	"github.com/cenkalti/backoff/v4"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
)

func (t *Authenticator) GetToken() (string, error) {
	if err := t.refreshToken(); err != nil {
		return "", err
	}
	return t.token, nil
}

func (t *Authenticator) refreshToken() error {
	t.mu.Lock()
	defer t.mu.Unlock()

	if time.Now().Before(t.expireDate) {
		return nil
	}

	getErr := t.getToken()
	if getErr == nil {
		return nil
	}

	if time.Now().Before(t.expireDeadline) {
		return nil
	}
	return getErr
}

const (
	tokenGetTimeout    = time.Millisecond * 500
	tokenRetryDelay    = time.Millisecond * 200
	tokenRetryMaxDelay = time.Millisecond * 2000
	tokenGetRetries    = 5
	tokenGetDelay      = time.Minute * 5
)

func (t *Authenticator) getToken() error {
	ctx, cm := tooling.InitContext(t.ctx, "cloudmeta")
	cm.InitTracing("GetMetadataToken")
	defer cm.FinishTracing()

	ebo := backoff.NewExponentialBackOff()
	ebo.InitialInterval = tokenRetryDelay
	ebo.MaxInterval = tokenRetryMaxDelay
	ebo.RandomizationFactor = 0.25
	bo := backoff.WithContext(backoff.WithMaxRetries(ebo, tokenGetRetries), ctx)

	retryCtx := tooling.StartRetry(ctx)
	return backoff.Retry(func() error {
		tooling.RetryIteration(retryCtx)
		return t.makeTokenRequest(retryCtx)
	}, bo)
}

func (t *Authenticator) makeTokenRequest(ctx context.Context) (err error) {
	token, err := t.metaClient.GetToken(ctx)
	if err != nil {
		return err
	}
	expireDeadline := time.Now().Add(token.ExpiresIn)
	expireDate := expireDeadline.Add(-tokenGetDelay)
	if expireDate.Before(time.Now()) {
		expireDate = expireDeadline
	}

	t.token = token.AccessToken
	t.expireDate = expireDate
	t.expireDeadline = expireDeadline
	t.refreshDate = time.Now()
	return nil
}
