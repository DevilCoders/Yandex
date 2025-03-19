package letsencrypt

import (
	"context"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type URLStorage interface {
	CertificateURL(ctx context.Context, host string) (string, error)
	SetCertificateURL(ctx context.Context, host, url string) error
	RemoveCertificateURL(ctx context.Context, host string) error
}

var (
	// ErrCertURLNotFound no certificate url for host
	ErrCertURLNotFound = xerrors.NewSentinel("certificate url not found")
)
