package restapi

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/crt/cloudcrt/generated/swagger/client/s_s_l_certificates"
	"a.yandex-team.ru/cloud/mdb/internal/crt/cloudcrt/generated/swagger/models"
)

func (c *Client) listCertificates(ctx context.Context, hostname string) (*models.ServerListCertificatesResponse, error) {
	params := s_s_l_certificates.NewGetAPICertificateParamsWithContext(ctx).
		WithAuthorization(c.addAuthHeaderVal()).WithHost(&hostname)
	respOk, err := c.api.SslCertificates.GetAPICertificate(params)
	if err != nil {
		return nil, apiError(err)
	}
	payload := respOk.GetPayload()
	return payload, nil
}
