package common

import (
	"a.yandex-team.ru/cloud/mdb/internal/ready"
)

type Health interface {
	ready.Checker
}
