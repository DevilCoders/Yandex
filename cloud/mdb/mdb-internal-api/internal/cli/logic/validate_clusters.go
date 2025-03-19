package logic

import (
	"context"
	"time"

	hasql "golang.yandex/hasql/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	intapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/app/mdb"
	kafka "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/provider"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// ValidateClusters iterates over all clusters of specified type, calls Validate() on their pillars and prints report
func ValidateClusters(ctx context.Context, cfg intapi.Config, logger log.Logger, clusterType string) error {
	handlers := map[string]struct {
		Type     clusters.Type
		Validate func(cluster metadb.Cluster) error
	}{
		"kafka": {
			Type:     clusters.TypeKafka,
			Validate: kafka.ValidateCluster,
		},
	}

	handler, ok := handlers[clusterType]
	if !ok {
		clusterTypes := make([]string, 0, len(handlers))
		for ct := range handlers {
			clusterTypes = append(clusterTypes, ct)
		}
		return xerrors.Errorf("unsupported cluster type: %s; valid types are: %v",
			clusterType, clusterTypes)
	}

	db, err := pg.New(cfg.MetaDB.DB, logger)
	if err != nil {
		return xerrors.Errorf("failed to initialize metadb backend: %w", err)
	}

	deadline := time.Now().Add(time.Second)
	for db.IsReady(ctx) != nil {
		if time.Now().After(deadline) {
			return xerrors.New("timed out waiting for metadb to become available")
		}
		time.Sleep(time.Millisecond)
	}

	ctx, err = db.Begin(ctx, hasql.Alive)
	if err != nil {
		return xerrors.Errorf("failed to begin metadb transaction: %w", err)
	}

	defer func() {
		if err := db.Rollback(ctx); err != nil {
			logger.Errorf("failed to rollback: %v", err)
		}
	}()

	var lastCid optional.String
	visibility := models.VisibilityVisible
	for {
		clusterIDs, err := db.ListClusterIDs(ctx, models.ListClusterIDsArgs{
			ClusterType:    handler.Type,
			Visibility:     visibility,
			Limit:          1000,
			AfterClusterID: lastCid,
		})
		if err != nil {
			return err
		}

		if len(clusterIDs) == 0 {
			break
		}

		for _, cid := range clusterIDs {
			lastCid = optional.NewString(cid)
			cluster, err := db.ClusterByClusterID(ctx, cid, visibility)
			if err != nil {
				return xerrors.Errorf("failed to fetch list of cluster ids: %w", err)
			}

			err = handler.Validate(cluster)
			if err != nil {
				logger.Warnf("Cluster %s is not valid: %s", cluster.ClusterID, err)
			} else {
				logger.Infof("Cluster %s is valid", cluster.ClusterID)
			}
		}
	}

	return nil
}
