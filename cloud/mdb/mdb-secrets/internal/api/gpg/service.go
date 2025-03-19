package gpg

import (
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/library/go/core/log"
)

// Service contains all handlers for gpg
type Service struct {
	secretsDB secretsdb.Service
	auth      httpauth.Authenticator
	lg        log.Logger
}

// New constructs gpg.Service
func New(secretsDB secretsdb.Service, auth httpauth.Authenticator, lg log.Logger) *Service {
	return &Service{secretsDB: secretsDB, auth: auth, lg: lg}
}
