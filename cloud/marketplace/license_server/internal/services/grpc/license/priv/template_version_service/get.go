package templateversionservice

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/priv"
	templateversions "a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/license/template_versions"
)

func (s *templateVersionService) Get(ctx context.Context, request *priv.GetTemplateVersionRequest) (*license.TemplateVersion, error) {
	res, err := templateversions.NewGetAction(s.Env).Do(ctx, templateversions.GetParams{ID: request.Id})

	if err := s.MapActionError(ctx, err); err != nil {
		return nil, err
	}

	ltv, err := res.TemplateVersion.Proto()
	if err != nil {
		return nil, err
	}

	return ltv, nil
}
