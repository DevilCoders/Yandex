package lockservice

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/priv"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/license/locks"
)

func (s *lockService) Get(ctx context.Context, request *priv.GetLockRequest) (*license.Lock, error) {
	res, err := locks.NewGetAction(s.Env).Do(ctx, locks.GetParams{
		InstanceID: request.InstanceId,
	})

	if err := s.MapActionError(ctx, err); err != nil {
		return nil, err
	}

	ll, err := res.Lock.Proto()
	if err != nil {
		return nil, err
	}

	return ll, nil
}
