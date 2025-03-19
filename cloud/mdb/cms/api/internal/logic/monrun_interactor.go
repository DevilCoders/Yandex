package logic

import (
	"context"
	"fmt"
	"time"

	"github.com/jonboulle/clockwork"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/duty"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/push"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type MonrunInteractor struct {
	log    log.Logger
	cmsdb  cmsdb.Client
	cfg    duty.CmsDom0DutyConfig
	dbm    dbm.Client
	pusher push.Pusher
	clock  clockwork.Clock
}

func NewMonrunInteractor(logger log.Logger, client cmsdb.Client, cfg duty.CmsDom0DutyConfig, dbm dbm.Client, pusher push.Pusher) MonrunInteractor {
	return MonrunInteractor{
		log:    logger,
		cmsdb:  client,
		cfg:    cfg,
		dbm:    dbm,
		pusher: pusher,
		clock:  clockwork.NewRealClock(),
	}
}

func (mr *MonrunInteractor) addEvents(events []push.Event, reqs []models.ManagementRequest, service string) []push.Event {
	for _, req := range reqs {
		events = append(events, push.Event{
			Host:        req.MustOneFQDN(),
			Service:     service,
			Status:      push.CRIT,
			Description: fmt.Sprintf("https://%s/cms/decision/?q=%s", mr.cfg.UIHost, req.ExtID),
		})
	}
	return events
}

func (mr *MonrunInteractor) SendEvents(ctx context.Context) error {
	reqsToConsider, err := mr.cmsdb.GetRequestsToConsider(ctx, mr.cfg.DutyDecisionThresholdMin)
	if err != nil {
		return xerrors.Errorf("GetRequestsToConsider: %w", err)
	}

	unfReqs, err := mr.cmsdb.GetUnfinishedRequests(ctx, time.Hour*time.Duration(mr.cfg.OkHoursUnfinished))
	if err != nil {
		return xerrors.Errorf("GetUnfinishedRequests: %w", err)
	}

	rupReqs, err := mr.cmsdb.GetResetupRequests(ctx, getResetupWindow(mr.cfg, mr.clock))
	if err != nil {
		return xerrors.Errorf("GetResetupRequests: %w", err)
	}

	rupReqs, err = filterContainersHosts(ctx, rupReqs, mr.dbm)
	if err != nil {
		return xerrors.Errorf("filter container hosts: %w", err)
	}

	request := push.Request{Events: make([]push.Event, 0, len(reqsToConsider)+len(unfReqs)+len(rupReqs))}
	request.Events = mr.addEvents(request.Events, reqsToConsider, "cms_consider_requests")
	request.Events = mr.addEvents(request.Events, unfReqs, "cms_unfinished_requests")
	request.Events = mr.addEvents(request.Events, rupReqs, "cms_resetup_requests")

	if err = mr.pusher.Push(ctx, request); err != nil {
		return xerrors.Errorf("push events: %w", err)
	}

	return nil
}

func getResetupWindow(cfg duty.CmsDom0DutyConfig, clock clockwork.Clock) time.Duration {
	var hours int
	switch clock.Now().Weekday() {
	case time.Friday, time.Saturday, time.Sunday:
		hours = cfg.OkFridayHoursAtWalle
	default:
		hours = cfg.OkHoursAtWalle
	}

	return time.Hour * time.Duration(hours)
}

func filterContainersHosts(ctx context.Context, reqs []models.ManagementRequest, dbmcl dbm.Client) ([]models.ManagementRequest, error) {
	var res []models.ManagementRequest
	for _, req := range reqs {
		c, err := dbmcl.Dom0Containers(ctx, req.MustOneFQDN())
		if err != nil && !xerrors.Is(err, dbm.ErrMissing) {
			return nil, err
		}
		if len(c) > 0 {
			res = append(res, req)
		}
	}
	return res, nil
}
