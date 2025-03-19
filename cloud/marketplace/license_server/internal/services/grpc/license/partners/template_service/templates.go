package templateservice

import (
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/partners"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/base"
)

type templateService struct {
	base.Service
}

func NewTemplateService(env *env.Env) *templateService {
	return &templateService{
		Service: base.Service{
			Env: env,
		},
	}
}

func (s *templateService) Bind(server *grpc.Server) {
	partners.RegisterTemplateServiceServer(server, s)
}
