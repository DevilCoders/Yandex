package auth

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/logs-api/internal/models"
)

//go:generate ../../../scripts/mockgen.sh Authenticator

type Authenticator interface {
	Authorize(ctx context.Context, resources []models.LogSource) error
}
