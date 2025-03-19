package factory

import (
	backupsprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/backups/provider"
	clustersprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters/provider"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters/provider/hostname"
	clustersecretsmock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters/provider/mocks"
	clustersecretsrsa "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters/provider/rsa"
	computeprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/compute/provider"
	eventsprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/events/provider"
	healthprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/health/provider"
	seachprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search/provider"
	sessionsprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions/provider"
	tasksprov "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks/provider"
)

var (
	NewOperator                    = clustersprov.NewOperator
	NewClusters                    = clustersprov.NewClusters
	NewBackups                     = backupsprov.NewBackups
	NewEvents                      = eventsprov.NewEvents
	NewSearch                      = seachprov.NewSearch
	NewSessions                    = sessionsprov.NewSessions
	NewTasks                       = tasksprov.NewTasks
	NewHealth                      = healthprov.NewHealth
	NewCompute                     = computeprov.NewCompute
	NewClusterSecretsGenerator     = clustersecretsrsa.New
	NewClusterSecretsGeneratorMock = clustersecretsmock.NewMockClusterSecrets
	NewMdbHostnameGenerator        = hostname.NewMdbHostnameGenerator
	NewDataCloudHostnameGenerator  = hostname.NewDataCloudHostnameGenerator
)

type ClusterSecrets clustersprov.ClusterSecrets
