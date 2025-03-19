package rm

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"

	resources "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/resourcemanager/v1"
)

type CloudService interface {
	GetPermissionStages(ctx context.Context, cloudID string) ([]string, error)
}

func (c *Client) GetPermissionStages(ctx context.Context, cloudID string) ([]string, error) {
	span, spanCtx := tracing.Start(ctx, "rm:GetPermissionStages")
	defer span.Finish()

	response, err := c.cloudService.GetPermissionStages(spanCtx, &resources.GetPermissionStagesRequest{
		CloudId: cloudID,
	})

	if err := c.mapError(ctx, err); err != nil {
		return nil, err
	}

	return response.GetPermissionStages(), nil
}
