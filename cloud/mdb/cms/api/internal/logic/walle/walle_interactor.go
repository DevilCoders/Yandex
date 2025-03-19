package walle

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/authentication"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/authorization"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/statemachine"
	dbm2 "a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
)

type WalleInteractor struct {
	log   log.Logger
	cmsdb cmsdb.Client
	dbm   dbm2.Client
	sm    statemachine.StateMachine

	ready.Checker
}

func authorize(ctx context.Context, u authentication.Result) error {
	if !authorization.IsAuthenticated(ctx, u) {
		return semerr.Authentication("user not authenticated")
	}
	if !authorization.IsWalleSystem(ctx, u) {
		return semerr.Authorization("you are not walle")
	}
	return nil
}

func (wi *WalleInteractor) IsReady(ctx context.Context) error {
	return wi.cmsdb.IsReady(ctx)
}

func NewWalleInteractor(logger log.Logger, client cmsdb.Client, dbmClient dbm2.Client) WalleInteractor {
	return WalleInteractor{
		log:   logger,
		cmsdb: client,
		dbm:   dbmClient,
		sm:    statemachine.NewStateMachine(logger, client),
	}
}
