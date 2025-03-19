package templateservice

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/priv"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/license/templates"
)

func (s *templateService) Get(ctx context.Context, request *priv.GetTemplateRequest) (*license.Template, error) {
	res, err := templates.NewGetAction(s.Env).Do(ctx, templates.GetParams{ID: request.Id})

	if err := s.MapActionError(ctx, err); err != nil {
		return nil, err
	}

	lt, err := res.Template.Proto()
	if err != nil {
		return nil, err
	}

	return lt, nil
}
