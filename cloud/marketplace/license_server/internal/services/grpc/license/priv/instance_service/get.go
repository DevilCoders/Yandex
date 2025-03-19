package instanceservice

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/priv"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/license/instances"
)

func (s *instanceService) Get(ctx context.Context, request *priv.GetInstanceRequest) (*license.Instance, error) {
	res, err := instances.NewGetAction(s.Env).Do(ctx, instances.GetParams{
		ID: request.Id,
	})

	if err := s.MapActionError(ctx, err); err != nil {
		return nil, err
	}

	li, err := res.Instance.Proto()
	if err != nil {
		return nil, err
	}

	return li, nil
}
