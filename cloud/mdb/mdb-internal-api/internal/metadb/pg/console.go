package pg

import (
	"context"
	"database/sql"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

var (
	querySelectResourcePresetsByClusterType = sqlutil.Stmt{
		Name: "SelectResourcePresetsByClusterType",
		// language=PostgreSQL
		Query: `
SELECT
    r.role,
    d.disk_type_ext_id as disk_type_id,
    g.name AS geo_name,
    CASE WHEN min(lower(r.disk_size_range)) IS NULL THEN NULL ELSE int8range(min(lower(r.disk_size_range)), max(upper(r.disk_size_range))) END AS disk_size_range,
    array_remove(array_agg(DISTINCT ds ORDER BY ds), NULL) AS disk_sizes,
    f.name AS preset_id,
    f.cpu_limit,
    CAST((f.cpu_guarantee * 100 / f.cpu_limit) AS int) AS cpu_fraction,
    f.io_cores_limit,
    f.gpu_limit,
    f.memory_limit,
    r.min_hosts,
    r.max_hosts,
    f.type,
    f.generation
FROM  dbaas.valid_resources r
    JOIN dbaas.flavors f ON (f.id = r.flavor)
    JOIN dbaas.geo g USING (geo_id)
    JOIN dbaas.disk_type d USING (disk_type_id)
    LEFT JOIN LATERAL unnest(r.disk_sizes) as ds ON TRUE
WHERE
    r.cluster_type = :cluster_type
    AND (r.feature_flag IS NULL OR r.feature_flag = ANY(:feature_flags))
GROUP BY
    r.cluster_type,
    r.role,
    d.disk_type_ext_id,
    f.id,
    g.name,
    f.cpu_limit,
    f.cpu_guarantee,
    f.io_cores_limit,
    f.gpu_limit,
    f.memory_limit,
    r.min_hosts,
    r.max_hosts,
    f.type,
    f.generation,
    f.name
ORDER BY
    f.type,
    generation DESC,
    f.cpu_limit,
    cpu_fraction,
    f.memory_limit,
    f.name
`,
	}

	querySelectResourcePresetsByCloudRegion = sqlutil.Stmt{
		Name: "SelectResourcePresetsByCloudRegion",
		// language=PostgreSQL
		Query: `
SELECT
    r.role,
    CASE WHEN min(lower(r.disk_size_range)) IS NULL THEN NULL ELSE int8range(min(lower(r.disk_size_range)), max(upper(r.disk_size_range))) END AS disk_size_range,
    array_remove(array_agg(DISTINCT ds ORDER BY ds), NULL) AS disk_sizes,
    r.default_disk_size,
    f.name AS preset_id,
    f.cpu_limit,
    CAST((f.cpu_guarantee * 100 / f.cpu_limit) AS int) AS cpu_fraction,
    f.io_cores_limit,
    f.gpu_limit,
    f.memory_limit,
    r.min_hosts,
    r.max_hosts,
    f.type,
    f.generation
FROM     dbaas.valid_resources r
    JOIN dbaas.flavors f ON (f.id = r.flavor)
    JOIN dbaas.geo g USING (geo_id)
    JOIN dbaas.regions reg USING (region_id)
    LEFT JOIN LATERAL unnest(r.disk_sizes) as ds ON TRUE
WHERE
    r.cluster_type = :cluster_type
    AND reg.name = :region
    AND reg.cloud_provider = :cloud
    AND (r.feature_flag IS NULL OR r.feature_flag = ANY(:feature_flags))
GROUP BY
    r.cluster_type,
    r.role,
    r.default_disk_size,
    f.id,
    reg.region_id,
    reg.cloud_provider,
    f.cpu_limit,
    f.cpu_guarantee,
    f.io_cores_limit,
    f.gpu_limit,
    f.memory_limit,
    r.min_hosts,
    r.max_hosts,
    f.type,
    f.name
ORDER BY
    f.type,
    f.cpu_limit,
    f.memory_limit,
    f.name
`,
	}

	querySelectClustersCountInFolder = sqlutil.Stmt{
		Name: "SelectClustersCountInFolder",
		Query: `
SELECT type AS type, count(*) AS count
FROM dbaas.clusters
WHERE folder_id = :folder_id AND code.visible(clusters)
GROUP BY type
`,
	}

	querySelectCloudsByClusterType = sqlutil.Stmt{
		Name: "SelectCloudsByClusterType",
		// language=PostgreSQL
		Query: `
SELECT
    r.cloud_provider as cloud,
    ARRAY_AGG(DISTINCT r.name) as regions
FROM
    dbaas.regions r
    JOIN dbaas.geo g USING (region_id)
    JOIN dbaas.valid_resources v USING (geo_id)
WHERE
    v.cluster_type = :cluster_type
GROUP BY
    r.cloud_provider
ORDER BY
    r.cloud_provider
LIMIT :page_size OFFSET :offset
`,
	}
)

type consoleResourcePreset struct {
	ExtID           string           `db:"preset_id"`
	Role            string           `db:"role"`
	DiskTypeExtID   string           `db:"disk_type_id"`
	Zone            string           `db:"geo_name"`
	DiskSizeRange   pgtype.Int8range `db:"disk_size_range"`
	DiskSizes       pgtype.Int8Array `db:"disk_sizes"`
	DefaultDiskSize sql.NullInt64    `db:"default_disk_size"`
	CPULimit        int64            `db:"cpu_limit"`
	CPUFraction     int64            `db:"cpu_fraction"`
	IoCoresLimit    int64            `db:"io_cores_limit"`
	GPULimit        int64            `db:"gpu_limit"`
	MemoryLimit     int64            `db:"memory_limit"`
	MinHosts        int64            `db:"min_hosts"`
	MaxHosts        int64            `db:"max_hosts"`
	FlavorType      string           `db:"type"`
	Generation      int64            `db:"generation"`
}

func (b *Backend) GetResourcePresetsByClusterType(ctx context.Context, clusterType clusters.Type, featureFlags []string) ([]consolemodels.ResourcePreset, error) {
	var modelResources []consolemodels.ResourcePreset
	parser := func(rows *sqlx.Rows) error {
		var res consoleResourcePreset
		if err := rows.StructScan(&res); err != nil {
			return err
		}
		preset, err := consoleResourcePresetFromDB(res)
		if err != nil {
			return err
		}
		modelResources = append(modelResources, preset)
		return nil
	}

	if featureFlags == nil {
		featureFlags = make([]string, 0)
	}
	var pgFeatureFlags pgtype.TextArray
	if err := pgFeatureFlags.Set(featureFlags); err != nil {
		return nil, err
	}
	args := map[string]interface{}{
		"cluster_type":  clusterType.Stringified(),
		"feature_flags": pgFeatureFlags,
	}
	_, err := sqlutil.QueryTx(
		ctx,
		querySelectResourcePresetsByClusterType,
		args,
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}
	return modelResources, nil
}

func (b *Backend) GetResourcePresetsByCloudRegion(ctx context.Context, cloudType environment.CloudType, region string, clusterType clusters.Type, featureFlags []string) ([]consolemodels.ResourcePreset, error) {
	var modelResources []consolemodels.ResourcePreset
	parser := func(rows *sqlx.Rows) error {
		var res consoleResourcePreset
		if err := rows.StructScan(&res); err != nil {
			return err
		}
		preset, err := consoleResourcePresetFromDB(res)
		if err != nil {
			return err
		}
		modelResources = append(modelResources, preset)
		return nil
	}

	if featureFlags == nil {
		featureFlags = make([]string, 0)
	}
	var pgFeatureFlags pgtype.TextArray
	if err := pgFeatureFlags.Set(featureFlags); err != nil {
		return nil, err
	}
	args := map[string]interface{}{
		"cluster_type":  clusterType.Stringified(),
		"feature_flags": pgFeatureFlags,
		"cloud":         cloudType,
		"region":        region,
	}
	_, err := sqlutil.QueryTx(
		ctx,
		querySelectResourcePresetsByCloudRegion,
		args,
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}
	return modelResources, nil
}

func rangeFromDB(dbRange pgtype.Int8range) consolemodels.Int64Range {
	result := consolemodels.Int64Range{}
	if dbRange.Status != pgtype.Present {
		return result
	}
	if dbRange.Lower.Status == pgtype.Present {
		result.Lower = dbRange.Lower.Int
		if dbRange.LowerType == pgtype.Exclusive {
			result.Lower = result.Lower + 1
		}
	}
	if dbRange.Upper.Status == pgtype.Present {
		result.Upper = dbRange.Upper.Int
		if dbRange.UpperType == pgtype.Exclusive {
			result.Upper = result.Upper - 1
		}
	}
	return result
}

func consoleResourcePresetFromDB(resources consoleResourcePreset) (consolemodels.ResourcePreset, error) {
	var sizes []int64
	if resources.DiskSizes.Status == pgtype.Present {
		sizes = make([]int64, 0)
		err := resources.DiskSizes.AssignTo(&sizes)
		if err != nil {
			return consolemodels.ResourcePreset{}, err
		}
	}

	role, err := hosts.ParseRole(resources.Role)
	if err != nil {
		return consolemodels.ResourcePreset{}, err
	}

	return consolemodels.ResourcePreset{
		ExtID:         resources.ExtID,
		Role:          role,
		Zone:          resources.Zone,
		CPULimit:      resources.CPULimit,
		CPUFraction:   resources.CPUFraction,
		MemoryLimit:   resources.MemoryLimit,
		DiskTypeExtID: resources.DiskTypeExtID,
		DiskSizes:     sizes,
		DiskSizeRange: rangeFromDB(resources.DiskSizeRange),
		DefaultDiskSize: optional.Int64{
			Valid: resources.DefaultDiskSize.Valid,
			Int64: resources.DefaultDiskSize.Int64,
		},
		MinHosts:   resources.MinHosts,
		MaxHosts:   resources.MaxHosts,
		FlavorType: resources.FlavorType,
		Generation: resources.Generation,
	}, nil
}

func (b *Backend) ClustersCountInFolder(ctx context.Context, folderID int64) (map[clusters.Type]int64, error) {
	counts := make(map[clusters.Type]int64)
	parser := func(rows *sqlx.Rows) error {
		var res struct {
			Type  string `db:"type"`
			Count int64  `db:"count"`
		}
		if err := rows.StructScan(&res); err != nil {
			return err
		}
		typ, err := clusters.ParseTypeStringified(res.Type)
		if err != nil {
			return err
		}

		counts[typ] = res.Count
		return nil
	}

	args := map[string]interface{}{
		"folder_id": folderID,
	}
	_, err := sqlutil.QueryTx(
		ctx,
		querySelectClustersCountInFolder,
		args,
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}

	return counts, nil
}

type consoleCloud struct {
	Cloud   string           `db:"cloud"`
	Regions pgtype.TextArray `db:"regions"`
}

func (b *Backend) GetCloudsByClusterType(ctx context.Context, clusterType clusters.Type, pageSize, offset int64) ([]consolemodels.Cloud, int64, error) {
	var clouds []consolemodels.Cloud
	parser := func(rows *sqlx.Rows) error {
		var res consoleCloud
		if err := rows.StructScan(&res); err != nil {
			return err
		}

		cloudType, err := environment.ParseCloudType(res.Cloud)
		if err != nil {
			return err
		}

		var regions []string
		if res.Regions.Status == pgtype.Present {
			regions = make([]string, 0)
			err := res.Regions.AssignTo(&regions)
			if err != nil {
				return nil
			}
		}

		clouds = append(clouds, consolemodels.Cloud{
			Cloud:   cloudType,
			Regions: regions,
		})
		return nil
	}

	args := map[string]interface{}{
		"cluster_type": clusterType.Stringified(),
		"page_size":    pageSize,
		"offset":       offset,
	}
	_, err := sqlutil.QueryTx(
		ctx,
		querySelectCloudsByClusterType,
		args,
		parser,
		b.logger,
	)

	return clouds, offset + int64(len(clouds)), err
}
