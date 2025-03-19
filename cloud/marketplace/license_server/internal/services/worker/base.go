package worker

import (
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
)

type baseService struct {
	*env.Env
}
