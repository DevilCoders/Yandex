package grpcserver

import (
	"context"
	"fmt"
	"strings"
	"time"

	api "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/alerting"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/push"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	rejectService  = "cms_instance_rejected_operations"
	staleService   = "cms_instance_stale_operations"
	unknownService = "cms_instance_unknown_fqdn"
)

func (s *InstanceService) Alarm(ctx context.Context, _ *api.AlarmRequest) (*api.AlarmResponse, error) {
	result := &api.AlarmResponse{
		Status:      api.AlarmResponse_OK,
		Description: "OK",
	}

	if time.Since(s.conductorCache.UpdatedAt()) > 10*time.Minute {
		result.Status = api.AlarmResponse_CRITICAL
		result.Description = "conductor cache is older than 10 minutes"
	}

	txCtx, err := s.cmsdb.Begin(ctx, sqlutil.PreferStandby)
	if err != nil {
		return nil, err
	}
	defer func() { _ = s.cmsdb.Rollback(txCtx) }()

	staleOps, err := s.cmsdb.StaleInstanceOperations(txCtx)
	if err != nil {
		return nil, err
	}

	staleOps, err = alerting.FilterOutAlertingOps(ctx, s.log, staleOps, s.cfg.Cms.Alerting, time.Now(), alerting.DefaultKnownConditions(s.mdb, s.cfg.Cms.Alerting.EnabledMW))
	if err != nil {
		return nil, err
	}

	alarmOps, err := s.cmsdb.InstanceOperationsToAlarm(txCtx)
	if err != nil {
		return nil, err
	}

	ops := append(staleOps, alarmOps...)

	if len(ops) == 0 {
		return result, nil
	}

	events, err := s.makeEvents(ops)
	if err != nil {
		result.Status = api.AlarmResponse_CRITICAL
		result.Description = fmt.Sprintf("can not create events: %v", err)
	} else {
		if err = s.pusher.Push(ctx, push.Request{Events: events}); err != nil {
			result.Status = api.AlarmResponse_CRITICAL
			result.Description = fmt.Sprintf("can not send events: %v", err)
		}
	}

	return result, nil
}

func (s *InstanceService) makeEvents(ops []models.ManagementInstanceOperation) ([]push.Event, error) {
	res := make([]push.Event, 0)
	unknownMessages := make([]string, 0)
	for _, op := range ops {
		fqdn := op.State.FQDN
		desc := fmt.Sprintf("https://%s/cms/instanceoperation/%s", s.cfg.UIHost, op.ID)

		if fqdn == "" {
			unknownMessages = append(unknownMessages, desc)
			continue
		}

		fqdn, err := s.converter.UnmanagedToManaged(fqdn)
		if err != nil {
			return nil, xerrors.Errorf("can not get unmanaged hostname: %w", err)
		}

		if _, ok := s.conductorCache.HostGroups(fqdn); !ok {
			unknownMessages = append(unknownMessages, desc)
			continue
		}

		var service string
		switch op.Status {
		case models.InstanceOperationStatusRejected:
			service = rejectService
		default:
			service = staleService
		}

		res = append(res, push.Event{
			Host:        fqdn,
			Service:     service,
			Status:      push.CRIT,
			Description: desc,
		})
	}

	unknownsRes := push.Event{
		Host:        s.cfg.FQDN,
		Service:     unknownService,
		Status:      push.OK,
		Description: "OK",
	}
	if len(unknownMessages) > 0 {
		unknownsRes.Status = push.CRIT
		unknownsRes.Description = strings.Join(unknownMessages, ", ")
	}
	res = append(res, unknownsRes)

	return res, nil
}

func (s *InstanceService) ChangeStatus(ctx context.Context, req *api.ChangeStatusRequest) (*api.ChangeStatusResponse, error) {
	txCtx, err := s.cmsdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return nil, err
	}
	defer func() { _ = s.cmsdb.Rollback(txCtx) }()

	op, err := s.cmsdb.GetInstanceOperation(txCtx, req.Id)
	if err != nil {
		return nil, err
	}

	switch req.Status {
	case api.ChangeStatusRequest_OK:
		op.Status = models.InstanceOperationStatusOK
	case api.ChangeStatusRequest_PROCESSING:
		op.Status = models.InstanceOperationStatusInProgress
	}

	err = s.cmsdb.UpdateInstanceOperationFields(txCtx, op)
	if err != nil {
		return nil, err
	}

	err = s.cmsdb.Commit(txCtx)
	if err != nil {
		return nil, err
	}

	return &api.ChangeStatusResponse{}, nil
}
