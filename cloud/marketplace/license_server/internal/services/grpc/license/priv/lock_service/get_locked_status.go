package lockservice

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/priv"
)

func (s *lockService) GetLockedStatus(ctx context.Context, request *priv.GetLockedStatusRequest) (*priv.GetLockedStatusResponse, error) {
	// TODO: support getLockedStatus action
	// res, err := locks.NewGetAction(s.Env).Do(ctx, locks.GetParams{
	// 	InstanceID: request.InstanceId,
	// 	ResourceLockID:    request.ResourceLockId,
	// })

	// if err := s.mapActionError(ctx, err); err != nil {
	// 	return nil, err
	// }

	// ll, err := licenseLockFromModel(res.Lock)
	// if err != nil {
	// 	return nil, err
	// }

	return &priv.GetLockedStatusResponse{
		ResouresLockedStatus: nil,
	}, nil
}
