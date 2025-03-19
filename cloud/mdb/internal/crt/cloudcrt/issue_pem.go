package restapi

import (
	"context"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/crt"
	"a.yandex-team.ru/cloud/mdb/internal/crt/cloudcrt/generated/swagger/client/s_s_l_certificates"
	"a.yandex-team.ru/cloud/mdb/internal/crt/cloudcrt/generated/swagger/models"
	"a.yandex-team.ru/library/go/ptr"
)

// https://wiki.yandex-team.ru/cloud/infra/crt/create_crt/
func (c *Client) IssueCert(ctx context.Context, target string, altNames []string, caName string, certType crt.CertificateType) (*crt.Cert, error) {
	id, err := c.Issue(ctx, target, caName, altNames, certType)
	if err != nil {
		return nil, err
	}
	return c.DownloadCertByID(ctx, id)
}

func (c *Client) Issue(ctx context.Context, target string, caName string, altNames []string, certType crt.CertificateType) (int64, error) {
	params := s_s_l_certificates.NewPostAPICertificateParamsWithContext(ctx).
		WithAuthorization(c.addAuthHeaderVal()).
		WithIssuecertificaterequest(&models.RequestsIssueReq{
			AbcService:     int64(c.cfg.AbcServiceID),
			CaName:         caName,
			DesiredTTLDays: certLivesDays,
			Hosts:          strings.Join(append(altNames, target), ","),
			Type:           ptr.String(string(certType)),
		})
	respOk, err := c.api.SslCertificates.PostAPICertificate(params)
	if err != nil {
		return 0, apiError(err)
	}
	payload := respOk.GetPayload()
	return payload.ID, nil
}
