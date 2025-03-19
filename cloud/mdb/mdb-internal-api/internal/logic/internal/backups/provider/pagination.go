package provider

import (
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/library/go/x/math"
)

func paginate(backups []bmodels.Backup, pageToken bmodels.BackupsPageToken, pageSize int64) (page []bmodels.Backup, nextPageToken bmodels.BackupsPageToken) {
	start := 0
	for i, b := range backups {
		if b.SourceClusterID == pageToken.ClusterID && b.ID == pageToken.BackupID {
			start = i + 1
			break
		}
	}
	end := math.MinInt(start+int(pageSize), len(backups))

	if end == 0 {
		nextPageToken = bmodels.BackupsPageToken{More: false}
	} else if end-1 < len(backups) {
		nextPageToken = bmodels.BackupsPageToken{
			BackupID:  backups[end-1].ID,
			ClusterID: backups[end-1].SourceClusterID,
			More:      end < len(backups),
		}
	}

	return backups[start:end], nextPageToken
}
