package cert

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/cloud/mdb/internal/crt"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/library/go/core/log"
)

// Service contains handlers for certs
type Service struct {
	secretsDB secretsdb.Service
	auth      httpauth.Authenticator
	crtClient crt.Client
	nowF      func() time.Time
	lg        log.Logger
}

// New constructs cert.Service
func New(secretsDB secretsdb.Service, auth httpauth.Authenticator, crt crt.Client, lg log.Logger, nowF func() time.Time) *Service {
	return &Service{secretsDB: secretsDB, auth: auth, crtClient: crt, lg: lg, nowF: nowF}
}
