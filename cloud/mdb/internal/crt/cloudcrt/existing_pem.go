package restapi

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/crt"
	"a.yandex-team.ru/cloud/mdb/internal/crt/cloudcrt/generated/swagger/models"
	"a.yandex-team.ru/library/go/slices"
)

func (c *Client) ExistingCert(ctx context.Context, target string, caName string, altNames []string) (*crt.Cert, error) {
	payload, err := c.listCertificates(ctx, target)
	if err != nil {
		return nil, err
	}
	if payload.Count == 0 {
		return nil, crt.ErrNoCerts
	}
	cer, err := pickCert(payload.Certs, caName, append(altNames, target))
	if err != nil {
		return nil, err
	}
	if cer == nil {
		return nil, crt.ErrNoCerts
	}
	return c.DownloadCertByID(ctx, cer.ID)
}

func pickCert(res []*models.DbCertInfo, caName string, hosts []string) (*models.DbCertInfo, error) {
	var newest *models.DbCertInfo
	for _, cert := range res {
		if cert.Download2 == "" ||
			cert.CaName != caName ||
			CertStatus(cert.Status) != CertStatusIssued ||
			cert.Revoked != "" ||
			len(hosts) != len(slices.IntersectStrings(cert.Hosts, hosts)) {
			continue //skip this cert
		}
		if newest == nil {
			newest = cert
			continue
		}
		endDateCurrent, err := time.Parse(time.RFC3339, newest.EndDate)
		if err != nil {
			return nil, err
		}
		endDateCand, err := time.Parse(time.RFC3339, cert.EndDate)
		if err != nil {
			return nil, err
		}
		if endDateCurrent.Before(endDateCand) {
			newest = cert
		}
	}

	return newest, nil
}
