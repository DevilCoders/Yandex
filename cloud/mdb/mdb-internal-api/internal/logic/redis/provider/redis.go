package provider

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/backups"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/compute"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/events"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/redis"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/redis/provider/internal/rpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

type Redis struct {
	operator clusterslogic.Operator
	backups  backups.Backups
	events   events.Events
	tasks    tasks.Tasks
	compute  compute.Compute
}

var _ redis.Redis = &Redis{}

func New(
	operator clusterslogic.Operator,
	backups backups.Backups,
	events events.Events,
	tasks tasks.Tasks,
	compute compute.Compute,
) *Redis {
	return &Redis{
		operator: operator,
		backups:  backups,
		events:   events,
		tasks:    tasks,
		compute:  compute,
	}
}

type cluster struct {
	clusters.Cluster
	Pillar *rpillars.Cluster
}
