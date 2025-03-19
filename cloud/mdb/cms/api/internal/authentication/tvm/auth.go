package tvm

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/authentication"
	"a.yandex-team.ru/cloud/mdb/internal/auth/tvm/tvmtool"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type TvmAuthenticator struct {
	client tvmtool.Client
}

type Creds struct {
	Ticket string
}

func (a *TvmAuthenticator) Auth(ctx context.Context, credentials interface{}) (authentication.Result, error) {
	c, ok := credentials.(Creds)
	if !ok {
		return NewAnonymousResult(), xerrors.New("bad credentials")
	}
	st, err := a.client.CheckServiceTicket(ctx, "", c.Ticket)
	if err != nil {
		return NewAnonymousResult(), err
	}
	return NewResult(st.SrcID), nil
}

func (a *TvmAuthenticator) IsReady(ctx context.Context) error {
	return a.client.Ping(ctx)
}
