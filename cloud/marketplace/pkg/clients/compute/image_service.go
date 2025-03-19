package compute

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/compute/v1"
	"a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/operation"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"
)

type ImageServiceClient interface {
	Create(ctx context.Context, in *compute.CreateImageRequest) (*operation.Operation, error)
}

func (c *Client) Create(ctx context.Context, in *compute.CreateImageRequest) (*operation.Operation, error) {
	span, spanCtx := tracing.Start(ctx, "compute:CreateImage")
	defer span.Finish()
	op, err := c.imageService.Create(spanCtx, in)

	if err := c.mapError(ctx, err); err != nil {
		return nil, err
	}

	return op, nil
}
