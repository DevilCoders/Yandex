package pg

import (
	"context"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
)

var (
	querySelectCloud = sqlutil.Stmt{
		Name: "SelectCloud",
		Query: `
SELECT
    cloud_id,
    cloud_ext_id,
    cpu_quota,
    gpu_quota,
    memory_quota,
    ssd_space_quota,
    hdd_space_quota,
    clusters_quota,
    memory_used,
    ssd_space_used,
    hdd_space_used,
    cpu_used,
    gpu_used,
    clusters_used,
    code.get_cloud_feature_flags(cloud_id) AS feature_flags
FROM
    code.get_cloud(
        i_cloud_id => :cloud_id,
        i_cloud_ext_id => :cloud_ext_id
    )`,
	}

	queryCreateCloud = sqlutil.Stmt{
		Name: "CreateCloud",
		Query: `
SELECT
    cloud_id,
    cloud_ext_id,
    cpu_quota,
    gpu_quota,
    memory_quota,
    ssd_space_quota,
    hdd_space_quota,
    clusters_quota,
    memory_used,
    ssd_space_used,
    hdd_space_used,
    cpu_used,
    gpu_used,
    clusters_used
  FROM code.add_cloud(
    i_cloud_ext_id => :cloud_ext_id,
    i_quota => code.make_quota(
        i_cpu       => CAST(:cpu_quota AS real),
        i_gpu       => CAST(:gpu_quota AS bigint),
        i_memory    => CAST(:memory_quota AS bigint),
        i_ssd_space => CAST(:ssd_space_quota AS bigint),
        i_hdd_space => CAST(:hdd_space_quota AS bigint),
        i_clusters  => CAST(:clusters_quota AS bigint)
    ),
    i_x_request_id => :x_request_id
)`,
	}

	queryLockCloud = sqlutil.Stmt{
		Name: "LockCloud",
		// language=PostgreSQL
		Query: `
SELECT
    cloud_id,
    cloud_ext_id,
    cpu_quota,
    gpu_quota,
    memory_quota,
    ssd_space_quota,
    hdd_space_quota,
    clusters_quota,
    memory_used,
    ssd_space_used,
    hdd_space_used,
    cpu_used,
    gpu_used,
    clusters_used,
    code.get_cloud_feature_flags(cloud_id) AS feature_flags
FROM code.lock_cloud(:cloud_id)
`,
	}

	querySetCloudQuota = sqlutil.Stmt{
		Name: "SetCloudQuota",
		// language=PostgreSQL
		Query: `
SELECT cloud_id,
       cloud_ext_id,
       cpu_quota,
       gpu_quota,
       memory_quota,
       ssd_space_quota,
       hdd_space_quota,
       clusters_quota,
       memory_used,
       ssd_space_used,
       hdd_space_used,
       cpu_used,
       gpu_used,
       clusters_used
FROM code.set_cloud_quota(
        i_cloud_ext_id => :cloud_ext_id,
        i_quota => code.make_quota(
                i_cpu => CAST(coalesce(:set_cpu, 0.0) AS real),
                i_gpu => CAST(coalesce(:set_gpu, 0) AS bigint),
                i_memory => CAST(coalesce(:set_memory, 0) AS bigint),
                i_ssd_space => CAST(coalesce(:set_ssd_space, 0) AS bigint),
                i_hdd_space => CAST(coalesce(:set_hdd_space, 0) AS bigint),
                i_clusters => CAST(coalesce(:set_clusters, 0) AS bigint)
            ),
        i_x_request_id => :x_request_id
    )
`,
	}

	queryUpdateCloudUsedQuota = sqlutil.Stmt{
		Name: "UpdateCloudUsedQuota",
		// language=PostgreSQL
		Query: `
SELECT
    cloud_id,
    cloud_ext_id,
    cpu_quota,
    gpu_quota,
    memory_quota,
    ssd_space_quota,
    hdd_space_quota,
    clusters_quota,
    memory_used,
    ssd_space_used,
    hdd_space_used,
    cpu_used,
    gpu_used,
    clusters_used
FROM code.update_cloud_usage(
    i_cloud_id   => :cloud_id,
    i_delta      => code.make_quota(
        i_cpu       => CAST(coalesce(:add_cpu, 0.0) AS real),
        i_gpu       => CAST(coalesce(:add_gpu, 0) AS bigint),
        i_memory    => CAST(coalesce(:add_memory, 0) AS bigint),
        i_ssd_space => CAST(coalesce(:add_ssd_space, 0) AS bigint),
        i_hdd_space => CAST(coalesce(:add_hdd_space, 0) AS bigint),
        i_clusters  => CAST(coalesce(:add_clusters, 0) AS bigint)
    ),
    i_x_request_id => :x_request_id
)
`,
	}

	queryGetDefaultFeatureFlags = sqlutil.Stmt{
		Name:  "GetDefaultFeatureFlags",
		Query: `SELECT flag_name FROM dbaas.default_feature_flags`,
	}
)

func (b *Backend) CloudByCloudID(ctx context.Context, cloudID string) (metadb.Cloud, error) {
	return b.queryCloud(ctx, querySelectCloud, map[string]interface{}{"cloud_id": cloudID, "cloud_ext_id": ""})
}

func (b *Backend) CloudByCloudExtID(ctx context.Context, cloudExtID string) (metadb.Cloud, error) {
	return b.queryCloud(ctx, querySelectCloud, map[string]interface{}{"cloud_ext_id": cloudExtID, "cloud_id": nil})
}

func (b *Backend) CreateCloud(ctx context.Context, cextid string, quota metadb.Resources, reqID string) (metadb.Cloud, error) {
	return b.queryCloud(
		ctx,
		queryCreateCloud,
		map[string]interface{}{
			"cloud_ext_id":    cextid,
			"cpu_quota":       quota.CPU,
			"gpu_quota":       quota.GPU,
			"memory_quota":    quota.Memory,
			"ssd_space_quota": quota.SSDSpace,
			"hdd_space_quota": quota.HDDSpace,
			"clusters_quota":  quota.Clusters,
			"x_request_id":    reqID,
		},
	)
}

func (b *Backend) LockCloud(ctx context.Context, cloudID int64) (metadb.Cloud, error) {
	return b.queryCloud(
		ctx,
		queryLockCloud,
		map[string]interface{}{
			"cloud_id": cloudID,
		},
	)
}

func (b *Backend) UpdateCloudQuota(ctx context.Context, cloudExtID string, quota metadb.Resources, reqID string) (metadb.Cloud, error) {
	return b.queryCloud(
		ctx,
		querySetCloudQuota,
		map[string]interface{}{
			"cloud_ext_id":  cloudExtID,
			"set_cpu":       quota.CPU,
			"set_gpu":       quota.GPU,
			"set_memory":    quota.Memory,
			"set_ssd_space": quota.SSDSpace,
			"set_hdd_space": quota.HDDSpace,
			"set_clusters":  quota.Clusters,
			"x_request_id":  reqID,
		},
	)
}

func (b *Backend) UpdateCloudUsedQuota(ctx context.Context, cloudID int64, quota metadb.Resources, reqID string) (metadb.Cloud, error) {
	return b.queryCloud(
		ctx,
		queryUpdateCloudUsedQuota,
		map[string]interface{}{
			"cloud_id":      cloudID,
			"add_cpu":       quota.CPU,
			"add_gpu":       quota.GPU,
			"add_memory":    quota.Memory,
			"add_ssd_space": quota.SSDSpace,
			"add_hdd_space": quota.HDDSpace,
			"add_clusters":  quota.Clusters,
			"x_request_id":  reqID,
		},
	)
}

func (b *Backend) DefaultFeatureFlags(ctx context.Context) ([]string, error) {
	var sourceFeatureFlags []string
	parser := func(rows *sqlx.Rows) error {
		var flag string
		if err := rows.Scan(&flag); err != nil {
			return err
		}

		sourceFeatureFlags = append(sourceFeatureFlags, flag)
		return nil
	}

	_, err := sqlutil.QueryTx(
		ctx,
		queryGetDefaultFeatureFlags,
		map[string]interface{}{},
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}

	return sourceFeatureFlags, nil
}

func (b *Backend) queryCloud(ctx context.Context, query sqlutil.Stmt, args map[string]interface{}) (metadb.Cloud, error) {
	var cloud Cloud
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&cloud)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		query,
		args,
		parser,
		b.logger,
	)
	if err != nil {
		return metadb.Cloud{}, wrapError(err)
	}
	if count == 0 {
		return metadb.Cloud{}, sqlerrors.ErrNotFound
	}

	return cloudFromDB(cloud), nil
}

type Cloud struct {
	CloudID       int64            `db:"cloud_id"`
	CloudExtID    string           `db:"cloud_ext_id"`
	CPUQuota      float64          `db:"cpu_quota"`
	GPUQuota      int64            `db:"gpu_quota"`
	MemoryQuota   int64            `db:"memory_quota"`
	SSDSpaceQuota int64            `db:"ssd_space_quota"`
	HDDSpaceQuota int64            `db:"hdd_space_quota"`
	ClustersQuota int64            `db:"clusters_quota"`
	CPUUsed       float64          `db:"cpu_used"`
	GPUUsed       int64            `db:"gpu_used"`
	MemoryUsed    int64            `db:"memory_used"`
	SSDSpaceUsed  int64            `db:"ssd_space_used"`
	HDDSpaceUsed  int64            `db:"hdd_space_used"`
	ClustersUsed  int64            `db:"clusters_used"`
	FeatureFlags  pgtype.TextArray `db:"feature_flags"`
}

func cloudFromDB(cloud Cloud) metadb.Cloud {
	c := metadb.Cloud{
		CloudID:    cloud.CloudID,
		CloudExtID: cloud.CloudExtID,
		Quota: metadb.Resources{
			CPU:      cloud.CPUQuota,
			GPU:      cloud.GPUQuota,
			Memory:   cloud.MemoryQuota,
			SSDSpace: cloud.SSDSpaceQuota,
			HDDSpace: cloud.HDDSpaceQuota,
			Clusters: cloud.ClustersQuota,
		},
		Used: metadb.Resources{
			CPU:      cloud.CPUUsed,
			GPU:      cloud.GPUUsed,
			Memory:   cloud.MemoryUsed,
			SSDSpace: cloud.SSDSpaceUsed,
			HDDSpace: cloud.HDDSpaceUsed,
			Clusters: cloud.ClustersUsed,
		},
	}

	for _, ff := range cloud.FeatureFlags.Elements {
		c.FeatureFlags = append(c.FeatureFlags, ff.String)
	}

	return c
}
