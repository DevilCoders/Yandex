package templateversionservice

import (
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/partners"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/base"
)

type templateVersionService struct {
	base.Service
}

func NewTemplateVersionService(env *env.Env) *templateVersionService {
	return &templateVersionService{
		Service: base.Service{
			Env: env,
		},
	}
}

func (s *templateVersionService) Bind(server *grpc.Server) {
	partners.RegisterTemplateVersionServiceServer(server, s)
}
