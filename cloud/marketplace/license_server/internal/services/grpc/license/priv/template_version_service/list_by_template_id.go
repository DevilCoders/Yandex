package templateversionservice

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/priv"
	templateversions "a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/license/template_versions"
	license_model "a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
)

func (s *templateVersionService) ListByTemplateID(ctx context.Context, request *priv.ListTemplateVersionByTemplateIDRequest) (*license.ListTemplateVersionResponse, error) {
	res, err := templateversions.NewListByTemplateIDAction(s.Env).Do(ctx, templateversions.ListByTemplateIDParams{
		TemplateID: request.TemplateId,
	})

	if err := s.MapActionError(ctx, err); err != nil {
		return nil, err
	}

	ltvs, err := license_model.TemplateVersionsListToProto(res.TemplateVersions)
	if err != nil {
		return nil, err
	}

	return &license.ListTemplateVersionResponse{TemplateVersions: ltvs}, nil
}
