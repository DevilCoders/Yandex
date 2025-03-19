package restapi

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/crt"
	"a.yandex-team.ru/cloud/mdb/internal/crt/cloudcrt/generated/swagger/client/s_s_l_certificates"
)

func (c *Client) DownloadCertByID(ctx context.Context, certID int64) (*crt.Cert, error) {
	params := s_s_l_certificates.NewGetAPICertificateIDDownloadPemParamsWithContext(ctx).
		WithAuthorization(c.addAuthHeaderVal()).
		WithID(certID)
	respOk, err := c.api.SslCertificates.GetAPICertificateIDDownloadPem(params)
	if err != nil {
		return nil, apiError(err)
	}
	return crt.ParseCert([]byte(respOk.GetPayload()))
}
