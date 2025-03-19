package pg

import (
	"context"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var querySelectUsedResources = sqlutil.Stmt{
	Name: "SelectUsedResources",
	Query: `
SELECT clo.cloud_ext_id,
	   clu.type,
	   s.roles[1] AS role,
	   f.platform_id,
	   CAST(sum(cpu_guarantee) AS bigint) AS cores,
	   sum(memory_guarantee) AS memory
  FROM dbaas.clusters clu
  JOIN dbaas.folders fo USING (folder_id)
  JOIN dbaas.clouds clo USING (cloud_id)
  JOIN dbaas.subclusters s USING (cid)
  JOIN dbaas.hosts h USING (subcid)
  JOIN dbaas.flavors f ON f.id = h.flavor
 WHERE dbaas.visible_cluster_status(clu.status)
   AND f.type != 'burstable'
   AND clu.type != 'hadoop_cluster'
   AND NOT (clu.type = 'clickhouse_cluster' AND s.roles[1] = 'zk')
   AND clo.cloud_ext_id = ANY(:cloud_ext_ids)
GROUP BY 1, 2, 3, 4
ORDER BY 1, 2, 3, 4`,
}

type usedResources struct {
	CloudID     string `db:"cloud_ext_id"`
	ClusterType string `db:"type"`
	Role        string
	PlatformID  string `db:"platform_id"`
	Cores       int64
	Memory      int64
}

func (b *Backend) GetUsedResources(ctx context.Context, clouds []string) ([]consolemodels.UsedResources, error) {
	var modelResources []consolemodels.UsedResources
	parser := func(rows *sqlx.Rows) error {
		var res usedResources
		if err := rows.StructScan(&res); err != nil {
			return err
		}

		r, err := resourcesFromDB(res)
		if err != nil {
			return err
		}

		modelResources = append(modelResources, r)
		return nil
	}

	var cloudExtIds pgtype.TextArray
	if err := cloudExtIds.Set(clouds); err != nil {
		return nil, xerrors.Errorf("set cloud ext ids: %w", err)
	}
	args := map[string]interface{}{
		"cloud_ext_ids": &cloudExtIds,
	}
	count, err := sqlutil.QueryTx(
		ctx,
		querySelectUsedResources,
		args,
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}
	if count == 0 {
		return nil, sqlerrors.ErrNotFound
	}

	return modelResources, nil
}

func resourcesFromDB(resources usedResources) (consolemodels.UsedResources, error) {
	typ, err := clusters.ParseTypeStringified(resources.ClusterType)
	if err != nil {
		return consolemodels.UsedResources{}, err
	}

	role, err := hosts.ParseRole(resources.Role)
	if err != nil {
		return consolemodels.UsedResources{}, err
	}

	return consolemodels.UsedResources{
		CloudID:     resources.CloudID,
		ClusterType: typ,
		Role:        role,
		PlatformID:  consolemodels.PlatformID(resources.PlatformID),
		Cores:       resources.Cores,
		Memory:      resources.Memory,
	}, nil
}
