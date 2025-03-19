package tvmticket

import (
	"context"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/auth"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type Authenticator struct {
	client tvm.Client
	dstID  tvm.ClientID

	ticket         string
	expireDate     time.Time
	expireDeadline time.Time
	refreshDate    time.Time

	ctx context.Context

	mu sync.Mutex
}

func New(ctx context.Context, client tvm.Client, dstID tvm.ClientID) *Authenticator {
	return &Authenticator{
		client: client,
		dstID:  dstID,
		ctx:    ctx,
	}
}

func (t *Authenticator) GRPCAuth() auth.GRPCAuthenticator {
	return &auth.GRPCTokenAuthenticator{TokenGetter: t}
}

func (t *Authenticator) YDBAuth() auth.YDBAuthenticator {
	return &auth.YDBTokenAuthenticator{TokenGetter: t}
}
