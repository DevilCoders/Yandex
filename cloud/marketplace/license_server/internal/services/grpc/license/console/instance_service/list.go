package instanceservice

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/console"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/license/instances"
)

func (s *instanceService) List(ctx context.Context, request *console.ListInstanceRequest) (*console.ListInstanceResponse, error) {
	res, err := instances.NewListAction(s.Env).Do(ctx, instances.ListParams{
		CloudID: request.CloudId,
	})

	if err := s.MapActionError(ctx, err); err != nil {
		return nil, err
	}

	lis := make([]*license.Instance, 0, len(res.Instances))
	for _, liModel := range res.Instances {
		li, err := liModel.Proto()
		if err != nil {
			return nil, err
		}
		lis = append(lis, li)
	}

	return &console.ListInstanceResponse{
		Instances: lis,
	}, nil
}
