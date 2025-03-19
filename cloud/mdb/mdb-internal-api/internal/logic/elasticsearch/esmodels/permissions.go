package esmodels

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

var (
	PermClustersGet = models.Permission{
		Name:   "managed-elasticsearch.clusters.get",
		Action: models.ASGet,
	}
	PermClustersCreate = models.Permission{
		Name:   "managed-elasticsearch.clusters.create",
		Action: models.ASCreate,
	}
	PermClustersUpdate = models.Permission{
		Name:   "managed-elasticsearch.clusters.update",
		Action: models.ASUpdate,
	}
	PermClustersDelete = models.NewPermissionWithForce(
		"managed-elasticsearch.clusters.delete",
		models.ASDelete,
		&models.Permission{
			Name:   "managed-elasticsearch.clusters.forceDelete",
			Action: models.ASDelete,
		},
	)
	PermClustersStart = models.Permission{
		Name:   "managed-elasticsearch.clusters.start",
		Action: models.ASUpdate,
	}
	PermClustersStop = models.Permission{
		Name:   "managed-elasticsearch.clusters.stop",
		Action: models.ASUpdate,
	}
)
