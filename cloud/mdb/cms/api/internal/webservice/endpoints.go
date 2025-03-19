package webservice

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/authentication"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/authentication/tvm"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/duty"
	metricsint "a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/metrics"
	walleint "a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/walle"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/push/http"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

// Service implements cms-deploy-api logic
type Service struct {
	lg log.Logger

	walle   walleint.WalleInteractor
	monrun  logic.MonrunInteractor
	auth    authentication.Authenticator
	metrics metricsint.Interactor
}

// NewService constructs Service
func NewService(db cmsdb.Client, dbm dbm.Client, lg log.Logger, auth authentication.Authenticator, cfg duty.CmsDom0DutyConfig) (*Service, error) {
	m := metricsint.GetInteractor(lg, db)
	pusher, err := http.NewLocalPusher(lg)
	if err != nil {
		return nil, err
	}
	srv := &Service{
		walle:   walleint.NewWalleInteractor(lg, db, dbm),
		monrun:  logic.NewMonrunInteractor(lg, db, cfg, dbm, pusher),
		auth:    auth,
		lg:      lg,
		metrics: *m,
	}

	return srv, nil
}

// IsReady checks is service is ready to serve requests
func (srv *Service) IsReady(ctx context.Context) error {
	if err := srv.walle.IsReady(ctx); err != nil {
		srv.lg.Warnf("db is not ready: %s", err)
		return err
	}

	if err := srv.auth.IsReady(ctx); err != nil {
		srv.lg.Warnf("auth subsystem is not ready: %s", err)
		return err
	}

	return nil
}

func (srv *Service) tvmAuth(ctx context.Context, yaServiceTicket string) (authentication.Result, error) {
	c := tvm.Creds{Ticket: yaServiceTicket}
	return srv.auth.Auth(ctx, c)
}

func (srv *Service) authenticate(ctx context.Context, credentials string) (authentication.Result, error) {
	result, err := srv.tvmAuth(ctx, credentials)
	if err != nil {
		ctxlog.Error(
			ctxlog.WithFields(ctx, log.Error(err)),
			srv.lg,
			"failed to authenticate user",
		)
		return result, semerr.Authentication("failed to authenticate")
	}
	return result, nil
}

func (srv *Service) GetRequests(ctx context.Context, credentials string) ([]models.ManagementRequest, error) {
	user, err := srv.authenticate(ctx, credentials)
	if err != nil {
		return []models.ManagementRequest{}, err
	}
	return srv.walle.GetPendingRequests(ctx, user)
}

func (srv *Service) RegisterRequestToManageNodes(
	ctx context.Context,
	credentials string,
	name string,
	taskID string,
	comment string,
	author string,
	taskType string,
	fqdns []string,
	extra interface{},
	failureType string,
	scenarioInfo models.ScenarioInfo,
	dryRun bool,
) (models.RequestStatus, error) {
	user, err := srv.authenticate(ctx, credentials)
	if err != nil {
		return models.StatusInProcess, err
	}
	return srv.walle.CreateRequest(ctx, user, name, taskID, comment, author, taskType, extra, fqdns, failureType, scenarioInfo, dryRun)
}

func (srv *Service) GetRequestStatus(ctx context.Context, credentials string, taskID string) (models.ManagementRequest, error) {
	user, err := srv.authenticate(ctx, credentials)
	if err != nil {
		return models.ManagementRequest{}, err
	}
	return srv.walle.GetRequest(ctx, user, taskID)
}

func (srv *Service) DeleteRequest(ctx context.Context, credentials string, taskID string) error {
	user, err := srv.authenticate(ctx, credentials)
	if err != nil {
		return err
	}
	return srv.walle.DeleteRequest(ctx, user, taskID)
}

func (srv *Service) SendEvents(ctx context.Context) error {
	return srv.monrun.SendEvents(ctx)
}
