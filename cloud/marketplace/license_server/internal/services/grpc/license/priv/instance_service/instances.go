package instanceservice

import (
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/priv"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/base"
)

type instanceService struct {
	base.Service
}

func NewInstanceService(env *env.Env) *instanceService {
	return &instanceService{
		Service: base.Service{
			Env: env,
		},
	}
}

func (s *instanceService) Bind(server *grpc.Server) {
	priv.RegisterInstanceServiceServer(server, s)
}
