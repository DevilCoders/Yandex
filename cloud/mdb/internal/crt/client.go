package crt

import (
	"context"
	"time"

	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../scripts/mockgen.sh Client

// Known errors
var (
	ErrNoCerts = xerrors.NewSentinel("no existing certs")
)

type CertificateType string

// Client is a certificator client
type Client interface {
	IssueCert(ctx context.Context, target string, altNames []string, caName string, certType CertificateType) (*Cert, error)
	ExistingCert(ctx context.Context, target string, caName string, altNames []string) (*Cert, error)
	RevokeCertsByHostname(ctx context.Context, hostname string) (int, error)
}

// Cert contains parts of certificator issued cert
type Cert struct {
	CertPem    []byte
	KeyPem     []byte
	Expiration time.Time
}
