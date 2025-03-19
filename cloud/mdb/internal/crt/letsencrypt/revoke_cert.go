package letsencrypt

import (
	"context"

	"golang.org/x/crypto/acme"

	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *LetsEncryptClient) RevokeCertsByHostname(ctx context.Context, hostname string) (int, error) {
	if err := c.register(ctx); err != nil {
		return 0, err
	}

	certURL, err := c.storage.CertificateURL(ctx, hostname)
	if err != nil {
		if xerrors.Is(err, ErrCertURLNotFound) {
			c.l.Warnf("certificate url for host %q not found, possible already deleted or not existed", hostname)
			return 0, nil
		}
		return 0, xerrors.Errorf("get certificate URL: %+v", err)
	}

	certs, err := c.acmeClient.FetchCert(ctx, certURL, false)
	if err != nil {
		return 0, xerrors.Errorf("fetch certificate: %+v", err)
	}

	var errs []error
	for _, cert := range certs {
		if err := c.acmeClient.RevokeCert(ctx, nil, cert, acme.CRLReasonUnspecified); err != nil {
			errs = append(errs, err)
		}
	}

	if errs != nil {
		return 0, xerrors.Errorf("revoke errors: %+v", errs)
	}

	if err := c.storage.RemoveCertificateURL(ctx, hostname); err != nil {
		c.l.Warnf("remove certificate from secretsdb %q, %+v", hostname, err)
	}

	return len(certs), nil
}
