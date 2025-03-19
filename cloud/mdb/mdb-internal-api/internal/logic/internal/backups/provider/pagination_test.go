package provider

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"

	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
)

func mkbackups(l int) (res []bmodels.Backup) {
	for i := 0; i < l; i++ {
		res = append(res, bmodels.Backup{
			ID:              fmt.Sprintf("backup_%d", i),
			SourceClusterID: "cluster_1",
		})
	}
	return
}

func TestPaginateEmptyToken(t *testing.T) {
	backups := mkbackups(10)
	chunk, nextPageToken := paginate(backups, bmodels.BackupsPageToken{}, 5)
	require.Len(t, chunk, 5, "page should have 5 items")
	require.Equal(t, chunk[0], backups[0], "first page item should be the first from list")
	require.Equal(t, chunk[4], backups[4], "last page item should be the 5th from list")
	require.Equal(t, nextPageToken.BackupID, backups[4].ID, "next page token should have correct BackupID field")
	require.Equal(t, nextPageToken.ClusterID, backups[4].SourceClusterID, "next page token should have correct BackupID field")
	require.True(t, nextPageToken.HasMore(), "next page token should have more items")
}

func TestPaginateWithToken(t *testing.T) {
	backups := mkbackups(10)
	pageToken := bmodels.BackupsPageToken{
		ClusterID: backups[2].SourceClusterID,
		BackupID:  backups[2].ID,
	}
	chunk, nextPageToken := paginate(backups, pageToken, 5)
	require.Len(t, chunk, 5, "page should have 5 items")
	require.Equal(t, chunk[0], backups[3], "first page item should be the 3rd from list")
	require.Equal(t, chunk[4], backups[7], "last page item should be the 7th from list")
	require.Equal(t, nextPageToken.BackupID, backups[7].ID, "next page token should have correct BackupID field")
	require.Equal(t, nextPageToken.ClusterID, backups[7].SourceClusterID, "next page token should have correct BackupID field")
	require.True(t, nextPageToken.HasMore(), "next page token should have more items")
}

func TestPaginateLastFullPage(t *testing.T) {
	backups := mkbackups(10)
	pageToken := bmodels.BackupsPageToken{
		ClusterID: backups[4].SourceClusterID,
		BackupID:  backups[4].ID,
	}
	chunk, nextPageToken := paginate(backups, pageToken, 5)
	require.Len(t, chunk, 5, "page should have 5 items")
	require.Equal(t, chunk[0], backups[5], "first page item should be the 5th from list")
	require.Equal(t, chunk[4], backups[9], "last page item should be the 9th from list")
	require.Equal(t, nextPageToken.BackupID, backups[9].ID, "next page token should have correct BackupID field")
	require.Equal(t, nextPageToken.ClusterID, backups[9].SourceClusterID, "next page token should have correct BackupID field")
	require.False(t, nextPageToken.HasMore(), "next page token should have no more items")
}

func TestPaginateLastIncompelePage(t *testing.T) {
	backups := mkbackups(10)
	pageToken := bmodels.BackupsPageToken{
		ClusterID: backups[7].SourceClusterID,
		BackupID:  backups[7].ID,
	}
	chunk, nextPageToken := paginate(backups, pageToken, 5)
	require.Len(t, chunk, 2, "page should have 2 items")
	require.Equal(t, chunk[0], backups[8], "first page item should be the 8th from list")
	require.Equal(t, chunk[1], backups[9], "last page item should be the 9th from list")
	require.Equal(t, nextPageToken.BackupID, backups[9].ID, "next page token should have correct BackupID field")
	require.Equal(t, nextPageToken.ClusterID, backups[9].SourceClusterID, "next page token should have correct BackupID field")
	require.False(t, nextPageToken.HasMore(), "next page token should have no more items")
}
