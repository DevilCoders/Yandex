package tvmticket

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"
)

var ErrTVMServiceBroken = errsentinel.New("tvm service is broken")

func (t *Authenticator) GetToken() (string, error) {
	if err := t.refreshToken(); err != nil {
		return "", err
	}
	return t.ticket, nil
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
	refreshTimeout  = time.Millisecond * 500
	refreshDelay    = time.Minute
	refreshInterval = time.Minute
)

func (t *Authenticator) getToken() error {
	ctx, cm := tooling.InitContext(t.ctx, "tvmapi")
	cm.InitTracing("GetTVMToken")
	defer cm.FinishTracing()

	return t.makeTokenRequest(ctx)
}

const ()

func (t *Authenticator) makeTokenRequest(ctx context.Context) (err error) {
	tooling.ICRequestWithNameStarted(ctx, "tvmapi", "GetToken")
	defer func() {
		if err != nil {
			err = ErrTVMServiceBroken.Wrap(err)
		}
		tooling.ICRequestDone(ctx, err)
	}()

	ctx, cancel := context.WithTimeout(ctx, refreshTimeout)
	defer cancel()

	t.ticket, err = t.client.GetServiceTicketForID(ctx, t.dstID)
	if err != nil {
		return
	}

	t.expireDate = time.Now().Add(refreshInterval)
	t.expireDeadline = t.expireDate.Add(refreshDelay)
	t.refreshDate = time.Now()
	return nil
}
