package restapi

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/internal/crt/cloudcrt/generated/swagger/client/s_s_l_certificates"
)

func (c *Client) RevokeCertByID(ctx context.Context, certID int64) error {
	params := s_s_l_certificates.NewDeleteAPICertificateIDParamsWithContext(ctx).
		WithAuthorization(c.addAuthHeaderVal()).
		WithID(certID)
	_, err := c.api.SslCertificates.DeleteAPICertificateID(params)
	return apiError(err)
}

// Revoke all certificates by hostname.
func (c *Client) RevokeCertsByHostname(ctx context.Context, hostname string) (int, error) {
	revokeCnt := 0
	res, err := c.listCertificates(ctx, hostname)
	if err != nil {
		return revokeCnt, err
	}
	for _, cert := range res.Certs {
		if CertStatus(cert.Status) == CertStatusIssued && cert.Revoked == "" {
			if err := c.RevokeCertByID(ctx, cert.ID); err != nil {
				return revokeCnt, fmt.Errorf("revoked %d of %d total certs: %w", revokeCnt, res.Count, err)
			}
			revokeCnt++
		}
	}
	return revokeCnt, nil
}
