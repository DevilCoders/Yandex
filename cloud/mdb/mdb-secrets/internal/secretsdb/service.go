package secretsdb

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../scripts/mockgen.sh Service

var (
	// ErrNoMaster is no master
	ErrNoMaster = xerrors.NewSentinel("no master available")
	// ErrNotAvailable no alive node available
	ErrNotAvailable = xerrors.NewSentinel("no alive node available")
	// ErrCertNotFound no certificate for host
	ErrCertNotFound = xerrors.NewSentinel("certificate not found")
	// ErrKeyNotFound gpg key for cid not found
	ErrKeyNotFound = xerrors.NewSentinel("key not found")
	// ErrCertAlreadyExist happens when inserting cert, but cert for host already exists
	ErrCertAlreadyExist = xerrors.NewSentinel("certificate for host already exists")
)

// Service is responsible for secretsdb communication
type Service interface {
	ready.Checker
	DeleteGpg(ctx context.Context, cid string) error
	PutGpg(ctx context.Context, cid, key string) (*EncryptedData, error)
	GetGpg(ctx context.Context, cid string) (*EncryptedData, error)

	DeleteCert(ctx context.Context, hostname string) error
	GetCert(ctx context.Context, host string) (*Cert, error)
	UpdateCert(ctx context.Context,
		hostname string,
		ca string,
		newCert func(ctx context.Context) (*CertUpdate, error),
		needUpdate func(cert *Cert) bool) (*Cert, error)
	//InsertDummyCert inserts initially expired cert so we can lock on it later
	InsertDummyCert(ctx context.Context, hostname string) error
	InsertNewCert(ctx context.Context, hostname string, ca string, altNames []string, cert CertUpdate) error

	CertificateURL(ctx context.Context, host string) (string, error)
	SetCertificateURL(ctx context.Context, host, url string) error
	RemoveCertificateURL(ctx context.Context, host string) error
}

// EncryptedData contains encrypted data and encryption version
type EncryptedData struct {
	Version int64  `json:"version"`
	Data    string `json:"data"`
}

// Cert database representation
type Cert struct {
	Host       string
	AltNames   []string
	Key        EncryptedData
	Crt        string
	Expiration time.Time
	CA         string
}

// CertUpdate contains values needed for certificate update
type CertUpdate struct {
	Key        string
	Crt        string
	Expiration time.Time
	AltNames   []string
}
