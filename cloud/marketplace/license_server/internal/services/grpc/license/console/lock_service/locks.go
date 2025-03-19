package lockservice

import (
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/console"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/base"
)

type lockService struct {
	base.Service
}

func NewLockService(env *env.Env) *lockService {
	return &lockService{
		Service: base.Service{
			Env: env,
		},
	}
}

func (s *lockService) Bind(server *grpc.Server) {
	console.RegisterLockServiceServer(server, s)
}
