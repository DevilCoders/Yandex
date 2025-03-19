package console

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
)

type RestoreResources struct {
	DiskSize         int64
	ResourcePresetID string
}

type RestoreHints struct {
	Version     string
	Environment environment.SaltEnv
	NetworkID   string
	Time        time.Time
}
