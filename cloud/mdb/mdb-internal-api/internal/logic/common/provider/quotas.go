package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Quotas struct {
	metadb metadb.Backend
	s      sessions.Sessions
	l      log.Logger
}

func adjustCurrentQuota(current metadb.Resources, changes common.BatchUpdateResources) metadb.Resources {
	result := current
	if changes.CPU.Valid {
		result.CPU = changes.CPU.Must()
	}
	if changes.GPU.Valid {
		result.GPU = changes.GPU.Must()
	}
	if changes.Memory.Valid {
		result.Memory = changes.Memory.Must()
	}
	if changes.SSDSpace.Valid {
		result.SSDSpace = changes.SSDSpace.Must()
	}
	if changes.HDDSpace.Valid {
		result.HDDSpace = changes.HDDSpace.Must()
	}
	if changes.Clusters.Valid {
		result.Clusters = changes.Clusters.Must()
	}
	return result
}

func (q *Quotas) Get(ctx context.Context, cloudExtID string) (metadb.Cloud, error) {
	ctx, _, err := q.s.Begin(ctx, sessions.ResolveByCloud(cloudExtID, models.PermMDBQuotasRead), sessions.WithPrimary())
	if err != nil {
		return metadb.Cloud{}, err
	}
	defer q.s.Rollback(ctx)

	cloudQuota, err := q.metadb.CloudByCloudExtID(ctx, cloudExtID)
	if err != nil {
		if xerrors.Is(err, sqlerrors.ErrNotFound) {
			return metadb.Cloud{}, semerr.WrapWithNotFound(err, "cloud not found")
		}
		return metadb.Cloud{}, err
	}

	if err = q.s.Commit(ctx); err != nil {
		return metadb.Cloud{}, err
	}

	return cloudQuota, err
}

func (q *Quotas) BatchUpdateMetric(ctx context.Context, cloudExtID string, quota common.BatchUpdateResources) error {
	ctx, _, err := q.s.Begin(ctx, sessions.ResolveByCloud(cloudExtID, models.PermMDBAllSupport), sessions.WithPrimary())
	if err != nil {
		return err
	}
	defer q.s.Rollback(ctx)

	cloud, err := q.metadb.CloudByCloudExtID(ctx, cloudExtID)
	if err != nil {
		if xerrors.Is(err, sqlerrors.ErrNotFound) {
			return semerr.WrapWithNotFound(err, "cloud not found")
		}
		return err
	}

	reqid := requestid.MustFromContext(ctx)
	_, err = q.metadb.UpdateCloudQuota(ctx, cloud.CloudExtID, adjustCurrentQuota(cloud.Quota, quota), reqid)
	if err != nil {
		return err
	}

	return q.s.Commit(ctx)
}

func NewQuotas(
	metadb metadb.Backend,
	s sessions.Sessions,
	l log.Logger,
) common.Quotas {
	return &Quotas{
		metadb: metadb,
		s:      s,
		l:      l,
	}
}
