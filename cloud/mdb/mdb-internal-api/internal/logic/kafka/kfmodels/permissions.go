package kfmodels

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

var (
	PermClustersGet = models.Permission{
		Name:   "managed-kafka.clusters.get",
		Action: models.ASGet,
	}
	PermClustersCreate = models.Permission{
		Name:   "managed-kafka.clusters.create",
		Action: models.ASCreate,
	}
	PermClustersUpdate = models.Permission{
		Name:   "managed-kafka.clusters.update",
		Action: models.ASUpdate,
	}
	PermClustersDelete = models.NewPermissionWithForce(
		"managed-kafka.clusters.delete",
		models.ASDelete,
		&models.Permission{
			Name:   "managed-kafka.clusters.forceDelete",
			Action: models.ASDelete,
		},
	)
	PermClustersStart = models.Permission{
		Name:   "managed-kafka.clusters.start",
		Action: models.ASUpdate,
	}
	PermClustersStop = models.Permission{
		Name:   "managed-kafka.clusters.stop",
		Action: models.ASUpdate,
	}
	PermSyncTopics = models.Permission{
		Name:   "managed-kafka.topics.sync",
		Action: models.ASUpdate,
	}
)
