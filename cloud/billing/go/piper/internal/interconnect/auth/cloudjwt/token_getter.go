package cloudjwt

import (
	"context"
	"errors"
	"fmt"
	"time"

	"github.com/cenkalti/backoff/v4"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/errtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"
	iam "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1"
)

var ErrJWTServiceBroken = errsentinel.New("jwt token service is broken")

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
	ctx, cm := tooling.InitContext(t.ctx, "cloudjwt")
	cm.InitTracing("GetJWTToken")

	ebo := backoff.NewExponentialBackOff()
	ebo.InitialInterval = tokenRetryDelay
	ebo.MaxInterval = tokenRetryMaxDelay
	ebo.RandomizationFactor = 0.25
	bo := backoff.WithContext(backoff.WithMaxRetries(ebo, tokenGetRetries), t.ctx)

	retryCtx := tooling.StartRetry(ctx)
	return backoff.Retry(func() error {
		tooling.RetryIteration(retryCtx)
		return t.makeTokenRequest(retryCtx)
	}, bo)
}

func (t *Authenticator) makeTokenRequest(ctx context.Context) (err error) {
	tooling.ICRequestWithNameStarted(ctx, "jwt-metadata", "CreateToken")
	defer func() {
		if err != nil {
			err = ErrJWTServiceBroken.Wrap(errtool.MapGRPCErr(ctx, err))
		}
		tooling.ICRequestDone(ctx, err)
	}()

	jwt, err := t.getJWT()
	if err != nil {
		return fmt.Errorf("create jwt: %w", err)
	}

	req := &iam.CreateIamTokenRequest{
		Identity: &iam.CreateIamTokenRequest_Jwt{Jwt: jwt},
	}

	var resp *iam.CreateIamTokenResponse
	resp, err = t.tokenClient.Create(t.ctx, req)
	if err != nil {
		return fmt.Errorf("create iam token request: %w", err)
	}
	if resp == nil {
		return errors.New("create iam token nil response")
	}

	if err := t.setToken(ctx, resp); err != nil {
		return fmt.Errorf("set iam token: %w", err)
	}
	return nil
}

func (t *Authenticator) setToken(ctx context.Context, resp *iam.CreateIamTokenResponse) error {
	if len(resp.IamToken) == 0 {
		return errors.New("got empty iam token")
	}

	expireDeadline := resp.ExpiresAt.AsTime()
	expireDate := expireDeadline.Add(-tokenGetDelay)
	if expireDate.Before(time.Now()) {
		expireDate = expireDeadline
	}

	t.token = resp.IamToken
	t.expireDate = expireDate
	t.expireDeadline = expireDeadline
	t.refreshDate = time.Now()
	return nil
}
