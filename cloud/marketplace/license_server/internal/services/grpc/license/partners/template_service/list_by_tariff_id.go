package templateservice

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/partners"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/license/templates"
	license_model "a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
)

func (s *templateService) ListByTariffID(ctx context.Context, request *partners.ListTemplateByTariffIDRequest) (*license.ListTemplateResponse, error) {
	res, err := templates.NewListByTariffIDAction(s.Env).Do(ctx, templates.ListByTariffIDParams{
		PublisherID: request.PublisherId,
		ProductID:   request.ProductId,
		TariffID:    request.TariffId,
	})

	if err := s.MapActionError(ctx, err); err != nil {
		return nil, err
	}

	lts, err := license_model.TemplatesListToProto(res.Templates)
	if err != nil {
		return nil, err
	}

	return &license.ListTemplateResponse{
		Templates: lts,
	}, nil
}
