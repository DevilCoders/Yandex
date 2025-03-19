package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
)

func (c *Console) FolderStats(ctx context.Context, folderExtID string) (console.FolderStats, error) {
	ctx, sess, err := c.sessions.Begin(ctx, sessions.ResolveByFolder(folderExtID, models.PermMDBAllRead), sessions.WithPrimary())
	if err != nil {
		return console.FolderStats{}, err
	}
	defer c.sessions.Rollback(ctx)

	counts, err := c.metaDB.ClustersCountInFolder(ctx, sess.FolderCoords.FolderID)
	if err != nil {
		return console.FolderStats{}, err
	}

	return console.FolderStats{Clusters: counts}, nil
}
