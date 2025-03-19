package walle

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/authentication"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

func (wi *WalleInteractor) GetRequest(ctx context.Context, user authentication.Result, taskID string) (models.ManagementRequest, error) {
	if err := authorize(ctx, user); err != nil {
		return models.ManagementRequest{}, err
	}
	requests, err := wi.cmsdb.GetRequestsByTaskID(ctx, []string{taskID})
	if err != nil {
		return models.ManagementRequest{}, err
	}
	req, ok := requests[taskID]
	if !ok {
		return req, semerr.NotFound("request not found")
	}
	return req, nil
}
