package healthiness

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
)

type FQDNCheck struct {
	Instance            models.Instance
	Cid                 string
	Sid                 string
	Roles               []string
	HACluster           bool
	HAShard             bool
	CntTotalInGroup     int
	CntAliveLeftInGroup int
	StatusUpdatedAt     time.Time
	HintIs              string
}
type HealthCheckResult struct {
	Unknown      []FQDNCheck
	Stale        []FQDNCheck
	WouldDegrade []FQDNCheck
	Ignored      []FQDNCheck
	GiveAway     []FQDNCheck
}
