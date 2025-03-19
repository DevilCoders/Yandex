package cloudmeta

import (
	"context"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/auth"
	cm "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/cloudmeta"
)

type Authenticator struct {
	token          string
	expireDate     time.Time
	expireDeadline time.Time
	refreshDate    time.Time

	ctx context.Context

	mu         sync.Mutex
	metaClient *cm.Client
}

func New(ctx context.Context) *Authenticator {
	return &Authenticator{
		ctx:        ctx,
		metaClient: cm.New(),
	}
}

func NewLocal(ctx context.Context) *Authenticator {
	return &Authenticator{
		ctx:        ctx,
		metaClient: cm.NewLocal(),
	}
}

func (t *Authenticator) GRPCAuth() auth.GRPCAuthenticator {
	return &auth.GRPCTokenAuthenticator{TokenGetter: t}
}

func (t *Authenticator) YDBAuth() auth.YDBAuthenticator {
	return &auth.YDBTokenAuthenticator{TokenGetter: t}
}
