package maintainer

import "a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/models"

func excludeClusters(src, toExclude []models.Cluster) []models.Cluster {
	exclusionMap := make(map[string]models.Cluster, len(toExclude))
	for _, cluster := range toExclude {
		exclusionMap[cluster.ID] = cluster
	}
	var result []models.Cluster
	for _, cluster := range src {
		if _, ok := exclusionMap[cluster.ID]; !ok {
			result = append(result, cluster)
		}
	}
	return result
}

func toClusterIDs(clusters []models.Cluster) []string {
	iDs := make([]string, 0, len(clusters))
	for _, cluster := range clusters {
		iDs = append(iDs, cluster.ID)
	}
	return iDs
}
