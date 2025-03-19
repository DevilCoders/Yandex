package yacrt

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/crt"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

// ExistingCert fetches latest certificate
func (c *Client) ExistingCert(ctx context.Context, target string, caName string, altNames []string) (*crt.Cert, error) {
	certInfo, err := c.latestCert(ctx, target, altNames, caName)
	if err != nil {
		return nil, xerrors.Errorf("certificate issue error: %w", err)
	}

	cert, err := c.downloadCert(ctx, certInfo.Download2)
	if err != nil {
		return nil, xerrors.Errorf("certificate download error: %w", err)
	}
	return crt.ParseCert(cert)
}

func (c *Client) latestCert(ctx context.Context, target string, altNames []string, caName string) (*certInfo, error) {
	certList, err := c.listCertificates(ctx, target)
	if err != nil {
		return nil, err
	}

	if certList.Count == 0 {
		return nil, crt.ErrNoCerts
	}
	cer := pickCert(certList.Results, caName, append(altNames, target))
	if cer == nil {
		return nil, crt.ErrNoCerts
	}
	return cer, nil
}

func pickCert(res []*certInfo, caName string, hosts []string) *certInfo {
	var newest *certInfo
	for _, cert := range res {
		if cert.Download2 == "" ||
			cert.CaName != caName ||
			cert.Status != CertStatusIssued ||
			cert.PrivKeyDeletedAt != "" ||
			cert.Revoked != "" ||
			len(hosts) != len(slices.IntersectStrings(cert.Hosts, hosts)) {
			continue //skip this cert
		}
		if newest == nil {
			newest = cert
			continue
		}
		if newest.EndDate.Before(cert.EndDate) {
			newest = cert
		}
	}

	return newest
}
