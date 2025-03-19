package walle

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/authentication"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

func (wi *WalleInteractor) GetPendingRequests(ctx context.Context, user authentication.Result) ([]models.ManagementRequest, error) {
	if err := authorize(ctx, user); err != nil {
		return nil, err
	}
	return wi.cmsdb.GetRequests(ctx)
}
