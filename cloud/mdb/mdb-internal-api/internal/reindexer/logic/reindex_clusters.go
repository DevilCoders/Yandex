package logic

import (
	"context"
	"time"

	hasql "golang.yandex/hasql/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	intapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/app/mdb"
	elasticsearch "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/provider"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/factory"
	greenplum "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum/provider"
	kafka "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/provider"
	opensearch "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch/provider"
	sqlserver "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/provider"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ClusterReindexer interface {
	ReindexCluster(ctx context.Context, cid string) error
}

type Reindexer struct {
	l                  log.Logger
	providers          map[clusters.Type]ClusterReindexer
	mdb                metadb.Backend
	maxSequentialFails int
}

func New(cfg intapi.Config, logger log.Logger, maxSequentialFails int) (*Reindexer, error) {
	db, err := pg.New(cfg.MetaDB.DB, logger)
	if err != nil {
		return nil, xerrors.Errorf("initialize metadb backend: %w", err)
	}
	search := factory.NewSearch(db)
	providers := map[clusters.Type]ClusterReindexer{
		clusters.TypeKafka:            kafka.NewReindexer(search),
		clusters.TypeElasticSearch:    elasticsearch.NewReindexer(search),
		clusters.TypeOpenSearch:       opensearch.NewReindexer(search),
		clusters.TypeGreenplumCluster: greenplum.NewReindexer(search),
		clusters.TypeSQLServer:        sqlserver.NewReindexer(search),
	}
	return &Reindexer{
		l:                  logger,
		mdb:                db,
		providers:          providers,
		maxSequentialFails: maxSequentialFails,
	}, nil
}

// Run reindex all supported clusters
func (r *Reindexer) Run(ctx context.Context) error {
	defer func() {
		if err := recover(); err != nil {
			sentry.CapturePanicAndWait(ctx, err, map[string]string{})
			panic(err)
		}
	}()
	if err := ready.WaitWithTimeout(ctx, time.Minute, r.mdb, &ready.DefaultErrorTester{L: r.l}, time.Second); err != nil {
		return xerrors.Errorf("not ready: %w", err)
	}

	ctx, err := r.mdb.Begin(ctx, hasql.Alive)
	if err != nil {
		return xerrors.Errorf("failed to begin metadb transaction: %w", err)
	}

	defer func() {
		if err := r.mdb.Rollback(ctx); err != nil {
			r.l.Warnf("rollback: %s", err)
		}
	}()

	var overallClusters, overallErrors, sequentialErrors int

	for clusterType, provider := range r.providers {
		r.l.Infof("Reindexing %q clusters", clusterType)
		var lastCid optional.String
		for {
			clusterIDs, err := r.mdb.ListClusterIDs(ctx, models.ListClusterIDsArgs{
				ClusterType:    clusterType,
				Visibility:     models.VisibilityVisible,
				Limit:          1000,
				AfterClusterID: lastCid,
			})
			if err != nil {
				return err
			}

			if len(clusterIDs) == 0 {
				r.l.Infof("Reindex of %q clusters finished", clusterType)
				break
			}

			lastCid.Set(clusterIDs[len(clusterIDs)-1])
			for _, cid := range clusterIDs {
				overallClusters++
				if err = func(cid string) error {
					// Create new context for each reindex,
					// cause transaction bound in context and we already have outer transaction
					reindexContext := context.Background()
					reindexContext, cancel := context.WithTimeout(reindexContext, time.Minute)
					defer cancel()
					reindexContext, err = r.mdb.Begin(reindexContext, hasql.Primary)
					if err != nil {
						return xerrors.Errorf("start TX for cluster reindex: %w", err)
					}
					defer func() {
						if rollErr := r.mdb.Rollback(reindexContext); rollErr != nil {
							r.l.Warnf("rollback: %s", rollErr)
						}
					}()
					err = provider.ReindexCluster(reindexContext, cid)
					if err != nil {
						return xerrors.Errorf("reindex cluster: %w", err)
					}
					if err := r.mdb.Commit(reindexContext); err != nil {
						return xerrors.Errorf("reindex failed on commit: %w", err)
					}
					r.l.Infof("Cluster %q reindex succeeded", cid)
					return nil
				}(cid); err != nil {
					overallErrors++
					sequentialErrors++
					r.l.Warnf("%q reindex failed: %s", cid, err)
					if !semerr.IsUnavailable(err) {
						sentry.GlobalClient().CaptureErrorAndWait(ctx, err, map[string]string{"cluster_id": cid})
					}
					if sequentialErrors >= r.maxSequentialFails {
						return xerrors.Errorf("fails %d times sequentially. Last error: %w", sequentialErrors, err)
					}
					continue
				}
				sequentialErrors = 0
			}
		}
	}
	r.l.Infof("reindex finished. Overall clusters: %d. Fails on %d clusters", overallClusters, overallErrors)
	return nil
}
