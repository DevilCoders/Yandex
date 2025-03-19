package pg

import (
	"context"
	"database/sql"
	"time"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	queryDiskTypes = sqlutil.Stmt{
		Name: "DiskTypes",
		Query: `
SELECT
    disk_type_ext_id,
    quota_type
FROM
    dbaas.disk_type
`,
	}

	queryValidResources = sqlutil.Stmt{
		Name: "GetValidResources",
		Query: `
WITH valid_resources AS (
    SELECT
        flavor,
        disk_type_id,
        geo_id,
        CASE WHEN min(lower(r.disk_size_range)) IS NULL THEN NULL ELSE int8range(min(lower(r.disk_size_range)), max(upper(r.disk_size_range))) END AS disk_size_range,
        array_remove(array_agg(DISTINCT ds ORDER BY ds), NULL) AS disk_sizes,
        min_hosts,
        max_hosts,
        array_agg(role ORDER BY role) as roles
    FROM
        dbaas.valid_resources r
        LEFT JOIN LATERAL unnest(r.disk_sizes) as ds ON TRUE
    WHERE
          cluster_type = :cluster_type
          AND ( :feature_flags IS NULL OR r.feature_flag IS NULL OR r.feature_flag = ANY(CAST(:feature_flags AS text[])) )
    GROUP BY
        flavor,
        disk_type_id,
        geo_id,
        min_hosts,
        max_hosts
)
SELECT
    f.name as id,
    CAST(vr.roles AS text[]) as roles,
    f.cpu_limit as cores,
    f.gpu_limit as gpus,
    f.memory_limit as memory,
    d.disk_type_ext_id as disk_type_id,
    array_agg(g.name) as zone_ids,
    vr.disk_size_range as disk_size_range,
    vr.disk_sizes as disk_sizes,
    vr.min_hosts as min_hosts,
    vr.max_hosts as max_hosts
FROM
    valid_resources vr
    JOIN dbaas.flavors f ON (f.id = vr.flavor)
    JOIN dbaas.geo g USING (geo_id)
    JOIN dbaas.disk_type d USING (disk_type_id)
WHERE
    (:role IS NULL OR :role = ANY(vr.roles))
    AND (:resource_preset_id IS NULL OR f.name = :resource_preset_id)
    AND (:geo IS NULL OR g.name = :geo)
    AND (:disk_type_ext_id IS NULL
         OR d.disk_type_ext_id = :disk_type_ext_id)
GROUP BY
    f.name,
    roles,
    cores,
    gpus,
    memory,
    disk_type_ext_id,
    disk_size_range,
    disk_sizes,
    min_hosts,
    max_hosts
ORDER BY
    cores,
    gpus,
    memory,
    f.name
`,
	}

	queryGetRegion = sqlutil.Stmt{
		Name: "GetRegion",
		Query: `
SELECT
    name,
    cloud_provider provider,
    timezone
FROM dbaas.regions
WHERE name = :name
`,
	}

	queryListZones = sqlutil.Stmt{
		Name: "ListZones",
		Query: `
SELECT
    g.name,
    r.name region,
    r.cloud_provider provider
FROM
    dbaas.geo g
    JOIN dbaas.regions r USING (region_id)
ORDER BY name
`,
	}

	querySelectFlavor = sqlutil.Stmt{
		Name: "SelectFlavor",
		// language=sql
		Query: `
SELECT
    d.disk_type_ext_id,
    CASE WHEN min(lower(r.disk_size_range)) IS NULL THEN NULL ELSE int8range(min(lower(r.disk_size_range)), max(upper(r.disk_size_range))) END AS disk_size_range,
    array_remove(array_agg(DISTINCT ds ORDER BY ds), NULL) AS disk_sizes,
    f.name AS id
FROM
    dbaas.valid_resources r
    JOIN dbaas.flavors f ON (f.id = r.flavor)
    JOIN dbaas.disk_type d USING (disk_type_id)
    JOIN dbaas.geo g USING (geo_id)
    LEFT JOIN LATERAL unnest(r.disk_sizes) as ds ON TRUE
WHERE
    r.cluster_type = :cluster_type
    AND ( :role IS NULL OR r.role = :role )
    AND ( :flavor_type IS NULL OR f.type = :flavor_type )
    AND ( :resource_preset_ext_id IS NULL OR f.name = :resource_preset_ext_id )
    AND ( :disk_type_ext_id IS NULL OR d.disk_type_ext_id = :disk_type_ext_id )
    AND ( :generation IS NULL OR f.generation = :generation )
    AND ( :min_cpu IS NULL OR f.cpu_limit >= :min_cpu )
    AND ( :feature_flags IS NULL OR r.feature_flag IS NULL OR r.feature_flag = ANY(:feature_flags) )
    AND ( :decommissioning_flavors IS NULL OR NOT f.name = ANY(:decommissioning_flavors) )
GROUP BY
    r.cluster_type,
    r.role,
    d.disk_type_ext_id,
    f.id
HAVING
    array_agg(g.name) @> :zones
ORDER BY
    f.vtype,
    f.type,
    f.generation DESC,
    f.cpu_limit,
    f.memory_limit,
    f.name
LIMIT 1
`,
	}

	querySelectDiskTypeByFlavor = sqlutil.Stmt{
		Name: "SelectDiskTypeByFlavor",
		Query: `
SELECT
    d.disk_type_ext_id
FROM
    dbaas.valid_resources r
    JOIN dbaas.flavors f ON (f.id = r.flavor)
    JOIN dbaas.disk_type d USING (disk_type_id)
    JOIN dbaas.geo g USING (geo_id)
WHERE
    r.cluster_type = :cluster_type
    AND r.role = :role
    AND f.name = :flavor_ext_id
    AND ( r.feature_flag IS NULL OR r.feature_flag = ANY(:feature_flags) )
GROUP BY
    d.disk_type_ext_id
HAVING
    array_agg(g.name) @> :zones
ORDER BY
    d.disk_type_ext_id DESC
LIMIT 1
`,
	}

	queryGetDiskIOLimit = sqlutil.Stmt{
		Name:  "GetDiskIOLimit",
		Query: `SELECT code.get_disk_limit(:space_limit, :disk_type, :resource_preset)`,
	}
)

var diskTypesMapping = map[string]resources.DiskType{
	"ssd": resources.DiskTypeSSD,
	"hdd": resources.DiskTypeHDD,
}

func (b *Backend) DiskTypes(ctx context.Context) (resources.DiskTypes, error) {
	mapping := make(map[string]resources.DiskType)
	parser := func(rows *sqlx.Rows) error {
		var v diskType
		if err := rows.StructScan(&v); err != nil {
			return err
		}

		dt, ok := diskTypesMapping[v.QuotaType]
		if !ok {
			sentry.GlobalClient().CaptureError(ctx, xerrors.Errorf("unknown disk type: %q", v.QuotaType), nil)
			return nil
		}

		mapping[v.DiskTypeExtID] = dt
		return nil
	}
	_, err := sqlutil.QueryTx(
		ctx,
		queryDiskTypes,
		nil,
		parser,
		b.logger,
	)
	if err != nil {
		return resources.DiskTypes{}, err
	}

	return resources.NewDiskTypes(mapping), nil
}

type validResources struct {
	ID            string           `db:"id"`
	Role          string           `db:"roles"`
	CPU           int              `db:"cores"`
	GPU           int              `db:"gpus"`
	Memory        int64            `db:"memory"`
	DiskTypeExtID string           `db:"disk_type_id"`
	ZoneID        string           `db:"zone_ids"`
	DiskSizeRange pgtype.Int8range `db:"disk_size_range"`
	DiskSizes     pgtype.Int8Array `db:"disk_sizes"`
	MinHosts      int64            `db:"min_hosts"`
	MaxHosts      int64            `db:"max_hosts"`
}

func (b *Backend) ValidResources(
	ctx context.Context,
	featureFlags []string,
	typ clusters.Type,
	role hosts.Role,
	resourcePresetExtID, diskTypeExtID, zoneID optional.String,
) ([]resources.Valid, error) {
	var res []resources.Valid
	parser := func(rows *sqlx.Rows) error {
		var vr validResources
		if err := rows.StructScan(&vr); err != nil {
			return err
		}

		v := resources.Valid{
			ID:            vr.ID,
			Role:          vr.Role,
			CPU:           vr.CPU,
			GPU:           vr.GPU,
			Memory:        vr.Memory,
			DiskTypeExtID: vr.DiskTypeExtID,
			ZoneID:        vr.ZoneID,
			MinHosts:      vr.MinHosts,
			MaxHosts:      vr.MaxHosts,
		}

		diskSizeRange, err := pgutil.OptionalIntervalInt64FromPGX(vr.DiskSizeRange)
		if err != nil {
			return err
		}
		v.DiskSizeRange = diskSizeRange

		v.DiskSizes = make([]int64, 0, len(vr.DiskSizes.Elements))
		for _, ds := range vr.DiskSizes.Elements {
			v.DiskSizes = append(v.DiskSizes, ds.Int)
		}

		if err = v.Validate(); err != nil {
			// Send error to sentry, ignore otherwise
			sentry.GlobalClient().CaptureError(ctx, err, nil)
			return nil
		}

		res = append(res, v)
		return nil
	}

	_, err := sqlutil.QueryTx(
		ctx,
		queryValidResources,
		map[string]interface{}{
			"cluster_type":       typ.Stringified(),
			"role":               role.Stringified(),
			"feature_flags":      featureFlags,
			"resource_preset_id": sql.NullString(resourcePresetExtID),
			"disk_type_ext_id":   sql.NullString(diskTypeExtID),
			"geo":                sql.NullString(zoneID),
		},
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}

	return res, nil
}

type region struct {
	Name      string `db:"name"`
	CloudType string `db:"provider"`
	TimeZone  string `db:"timezone"`
}

func (b *Backend) RegionByName(ctx context.Context, name string) (environment.Region, error) {
	var result environment.Region
	parser := func(rows *sqlx.Rows) error {
		var region region
		if err := rows.StructScan(&region); err != nil {
			return err
		}

		cloudType, err := environment.ParseCloudType(region.CloudType)
		if err != nil {
			return err
		}

		timeZone, err := time.LoadLocation(region.TimeZone)
		if err != nil {
			return err
		}

		result = environment.Region{
			Name:      region.Name,
			CloudType: cloudType,
			TimeZone:  timeZone,
		}

		return nil
	}
	_, err := sqlutil.QueryTx(
		ctx,
		queryGetRegion,
		map[string]interface{}{
			"name": name,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return environment.Region{}, err
	}

	return result, nil
}

type zone struct {
	Name          string `db:"name"`
	RegionID      string `db:"region"`
	CloudProvider string `db:"provider"`
}

func (b *Backend) ListZones(ctx context.Context) ([]environment.Zone, error) {
	var result []environment.Zone
	parser := func(rows *sqlx.Rows) error {
		var zone zone
		if err := rows.StructScan(&zone); err != nil {
			return err
		}

		cloudType, err := environment.ParseCloudType(zone.CloudProvider)
		if err != nil {
			return err
		}

		result = append(result, environment.Zone{
			Name:      zone.Name,
			RegionID:  zone.RegionID,
			CloudType: cloudType,
		})

		return nil
	}
	_, err := sqlutil.QueryTx(
		ctx,
		queryListZones,
		nil,
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}

	return result, nil
}

type defaultPreset struct {
	DiskTypeExtID string           `db:"disk_type_ext_id"`
	DiskSizeRange pgtype.Int8range `db:"disk_size_range"`
	DiskSizes     pgtype.Int8Array `db:"disk_sizes"`
	ID            string           `db:"id"`
}

func (b *Backend) GetResourcePreset(ctx context.Context, clusterType clusters.Type, role hosts.Role, flavorType optional.String,
	resourcePresetExtID, diskTypeExtID optional.String, generation optional.Int64, minCPU optional.Float64,
	zones, featureFlags, decommissionedResourcePresets []string) (resources.DefaultPreset, error) {
	var dp defaultPreset
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&dp)
	}

	var pgZones pgtype.TextArray
	if err := pgZones.Set(zones); err != nil {
		return resources.DefaultPreset{}, err
	}
	var pgFlags pgtype.TextArray
	if err := pgFlags.Set(featureFlags); err != nil {
		return resources.DefaultPreset{}, err
	}
	var pgPresets pgtype.TextArray
	if decommissionedResourcePresets == nil {
		decommissionedResourcePresets = []string{}
	}
	if err := pgPresets.Set(decommissionedResourcePresets); err != nil {
		return resources.DefaultPreset{}, err
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectFlavor,
		map[string]interface{}{
			"cluster_type":            clusterType.Stringified(),
			"role":                    role.Stringified(),
			"flavor_type":             sql.NullString{String: flavorType.String, Valid: flavorType.Valid},
			"resource_preset_ext_id":  sql.NullString{String: resourcePresetExtID.String, Valid: resourcePresetExtID.Valid},
			"disk_type_ext_id":        sql.NullString{String: diskTypeExtID.String, Valid: diskTypeExtID.Valid},
			"generation":              sql.NullInt64{Int64: generation.Int64, Valid: generation.Valid},
			"min_cpu":                 sql.NullFloat64{Float64: minCPU.Float64, Valid: minCPU.Valid},
			"zones":                   pgZones,
			"feature_flags":           pgFlags,
			"decommissioning_flavors": pgPresets,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return resources.DefaultPreset{}, err
	}
	if count == 0 {
		return resources.DefaultPreset{}, xerrors.Errorf("resource preset for cluster type %q with role %q %w",
			clusterType.Stringified(), role.Stringified(), sqlerrors.ErrNotFound)
	}

	result := resources.DefaultPreset{
		DiskTypeExtID: dp.DiskTypeExtID,
		ExtID:         dp.ID,
	}

	diskSizeRange, err := pgutil.OptionalIntervalInt64FromPGX(dp.DiskSizeRange)
	if err != nil {
		return resources.DefaultPreset{}, err
	}
	result.DiskSizeRange = diskSizeRange

	result.DiskSizes = make([]int64, 0, len(dp.DiskSizes.Elements))
	for _, ds := range dp.DiskSizes.Elements {
		result.DiskSizes = append(result.DiskSizes, ds.Int)
	}

	return result, nil
}

func (b *Backend) DiskTypeExtIDByResourcePreset(ctx context.Context, clusterType clusters.Type, role hosts.Role, resourcePreset string,
	zones []string, featureFlags []string) (string, error) {
	var diskType string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&diskType)
	}

	var pgZones pgtype.TextArray
	if err := pgZones.Set(zones); err != nil {
		return "", err
	}
	var pgFlags pgtype.TextArray
	if err := pgFlags.Set(featureFlags); err != nil {
		return "", err
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectDiskTypeByFlavor,
		map[string]interface{}{
			"cluster_type":  clusterType.Stringified(),
			"role":          role.Stringified(),
			"flavor_ext_id": resourcePreset,
			"zones":         pgZones,
			"feature_flags": pgFlags,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return "", err
	}
	if count == 0 {
		return "", xerrors.Errorf("disk type for resource preset %q in zones %v %w", resourcePreset, zones, sqlerrors.ErrNotFound)
	}

	return diskType, nil
}

func (b *Backend) DiskIOLimit(ctx context.Context, spaceLimit int64, diskType string, resourcePreset string) (int64, error) {
	var ioLimit int64
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&ioLimit)
	}

	_, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		queryGetDiskIOLimit,
		map[string]interface{}{
			"space_limit":     spaceLimit,
			"disk_type":       diskType,
			"resource_preset": resourcePreset,
		},
		parser,
		b.logger,
	)

	return ioLimit, wrapError(err)
}
