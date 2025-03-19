package pg

import (
	"context"
	"database/sql"
	"time"

	"github.com/jackc/pgtype"
	uuid "github.com/jackc/pgtype/ext/gofrs-uuid"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	queryAddHost = sqlutil.Stmt{
		Name: "AddHost",
		Query: `
SELECT subcid,
	   shard_id,
	   space_limit,
       flavor_id AS flavor_id,
       geo,
	   fqdn,
       disk_type_id,
       subnet_id,
       assign_public_ip
  FROM code.add_host(
    i_subcid => :subcid,
    i_shard_id => :shard_id,
    i_space_limit => :space_limit,
    i_flavor_id => :flavor_id,
    i_geo => :geo,
    i_fqdn => :fqdn,
    i_disk_type => :disk_type_id,
    i_subnet_id => :subnet_id,
    i_assign_public_ip => :assign_public_ip,
    i_cid => :cid,
    i_rev => :rev
)`,
	}

	queryModifyHost = sqlutil.Stmt{
		Name: "ModifyHost",
		Query: `
SELECT fqdn,
       subcid,
       shard_id,
       space_limit,
       flavor_id AS flavor_id,
       vtype,
       vtype_id,
       disk_type_id,
       assign_public_ip
  FROM code.update_host(
    i_fqdn => :fqdn,
    i_cid => :cid,
    i_rev => :rev,
    i_space_limit => :space_limit,
    i_flavor_id => :flavor_id,
    i_disk_type => :disk_type_id
)`,
	}

	queryModifyHostPublicIP = sqlutil.Stmt{
		Name: "ModifyHost",
		Query: `
SELECT fqdn,
       subcid,
       shard_id,
       space_limit,
       flavor_id AS flavor_id,
       vtype,
       vtype_id,
       disk_type_id,
       assign_public_ip
  FROM code.update_host(
    i_fqdn => :fqdn,
    i_cid => :cid,
    i_rev => :rev,
	i_assign_public_ip => :assign_public_ip
)`,
	}

	queryDeleteHosts = sqlutil.Stmt{
		Name: "DeleteHosts",
		Query: `
SELECT fqdn,
       subcid,
       shard_id,
       space_limit,
       flavor_id,
       vtype,
       vtype_id,
       disk_type_id,
       assign_public_ip
  FROM code.delete_hosts(
      i_fqdns => :fqdns,
      i_cid   => :cid,
      i_rev   => :rev
  )`,
	}

	querySelectClusterHosts = sqlutil.Stmt{
		Name: "SelectClusterHosts",
		// language=PostgreSQL
		Query: `
SELECT
    h.subcid,
    h.shard_id,
    sh.name AS shard_name,
    space_limit,
    flavor_id,
    geo,
    fqdn,
    assign_public_ip,
    f.name AS flavor_name,
    h.vtype,
    vtype_id,
    disk_type_id,
    subnet_id,
    cast(roles AS text[]) AS roles
FROM code.get_hosts_by_cid(:cid) h
JOIN dbaas.flavors f
  ON f.id = h.flavor_id
LEFT JOIN dbaas.shards sh
  ON sh.shard_id = h.shard_id
WHERE (:role IS NULL OR :role = ANY(roles))
  AND (:subcid IS NULL OR :subcid = h.subcid)
ORDER BY fqdn
LIMIT :limit
OFFSET :offset
 `,
	}

	querySelectClusterHostsAtRevision = sqlutil.Stmt{
		Name: "SelectClusterHostsAtRevision",
		Query: `
SELECT
    h.subcid,
    h.shard_id,
    sh.name AS shard_name,
    space_limit,
    h.flavor AS flavor_id,
    g.name AS geo,
    fqdn,
    assign_public_ip,
    f.name AS flavor_name,
    f.vtype,
    vtype_id,
    disk_type_ext_id AS disk_type_id,
    subnet_id,
    cast(roles AS text[]) AS roles
  FROM dbaas.subclusters_revs s
  JOIN dbaas.hosts_revs h
 USING (subcid, rev)
  JOIN dbaas.geo g
 USING (geo_id)
  JOIN dbaas.disk_type d
 USING (disk_type_id)
  JOIN dbaas.flavors f
    ON (h.flavor = f.id)
  LEFT JOIN dbaas.shards_revs sh
    ON (sh.subcid = s.subcid AND sh.rev = s.rev)
 WHERE s.cid = :cid
   AND (:role IS NULL OR :role = ANY(roles))
   AND (:shard_name IS NULL OR :shard_name = sh.name)
   AND s.rev = :revision
 `,
	}

	queryGetHostsByShard = sqlutil.Stmt{
		Name: "queryGetHostsByShard",
		// language=SQL
		Query: `
SELECT
	h.subcid,
	h.shard_id,
	s.name AS shard_name,
	flavor_id,
	f.name AS flavor_name,
	space_limit,
	geo,
	fqdn,
	disk_type_id,
	subnet_id,
	assign_public_ip,
	h.vtype,
	vtype_id,
	cast(roles AS text[]) AS roles
FROM code.get_hosts_by_shard(i_shard_id => :shard_id) h
JOIN dbaas.flavors f ON f.id = h.flavor_id
LEFT JOIN dbaas.shards s USING (shard_id)
ORDER BY fqdn`,
	}
)

func (b *Backend) AddHost(ctx context.Context, args models.AddHostArgs) (hosts.Host, error) {
	var h host
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&h)
	}

	var flavorID uuid.UUID
	if err := flavorID.Set(args.ResourcePresetID); err != nil {
		return hosts.Host{}, xerrors.Errorf("set resource preset id: %w", err)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryAddHost,
		map[string]interface{}{
			"subcid":           args.SubClusterID,
			"shard_id":         sql.NullString(args.ShardID),
			"space_limit":      args.SpaceLimit,
			"flavor_id":        flavorID,
			"geo":              args.ZoneID,
			"fqdn":             args.FQDN,
			"disk_type_id":     args.DiskTypeExtID,
			"subnet_id":        args.SubnetID,
			"assign_public_ip": args.AssignPublicIP,
			"cid":              args.ClusterID,
			"rev":              args.Revision,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return hosts.Host{}, err
	}
	if count == 0 {
		return hosts.Host{}, sqlerrors.ErrNotFound
	}

	return hostFromDB(h, args.ClusterID)
}

func (b *Backend) ModifyHost(ctx context.Context, args models.ModifyHostArgs) (hosts.Host, error) {
	var h host
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&h)
	}

	var flavorID uuid.UUID
	if err := flavorID.Set(args.ResourcePresetID); err != nil {
		return hosts.Host{}, xerrors.Errorf("set resource preset id: %w", err)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryModifyHost,
		map[string]interface{}{
			"fqdn":         args.FQDN,
			"cid":          args.ClusterID,
			"rev":          args.Revision,
			"space_limit":  args.SpaceLimit,
			"flavor_id":    flavorID,
			"disk_type_id": args.DiskTypeExtID,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return hosts.Host{}, err
	}
	if count == 0 {
		return hosts.Host{}, sqlerrors.ErrNotFound
	}

	return hostFromDB(h, args.ClusterID)
}

func (b *Backend) ModifyHostPublicIP(ctx context.Context, clusterID string, fqdn string, revision int64, assignPublicIP bool) (hosts.Host, error) {
	var h host
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&h)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryModifyHostPublicIP,
		map[string]interface{}{
			"fqdn":             fqdn,
			"cid":              clusterID,
			"rev":              revision,
			"assign_public_ip": assignPublicIP,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return hosts.Host{}, err
	}
	if count == 0 {
		return hosts.Host{}, sqlerrors.ErrNotFound
	}

	return hostFromDB(h, clusterID)
}

func (b *Backend) listHosts(ctx context.Context, query sqlutil.Stmt, args ListHostsArgs) (hosts []hosts.Host, nextPageToken int64, more bool, err error) {
	var dbHosts []host
	parser := func(rows *sqlx.Rows) error {
		var h host
		if err := rows.StructScan(&h); err != nil {
			return err
		}

		dbHosts = append(dbHosts, h)
		return nil
	}

	argMap := args.toMap()

	count, err := sqlutil.QueryTx(
		ctx,
		query,
		argMap,
		parser,
		b.logger,
	)
	if err != nil {
		return nil, 0, false, err
	}

	for _, h := range dbHosts {
		host, err := hostFromDB(h, args.ClusterID)
		if err != nil {
			return nil, 0, false, err
		}
		hosts = append(hosts, host)
	}

	if argMap["offset"] != nil {
		nextPageToken = args.Offset + count
	}
	if argMap["limit"] != nil {
		if count >= args.PageSize {
			more = true
		}
	}

	return hosts, nextPageToken, more, nil
}

func (b *Backend) ListHosts(ctx context.Context, clusterID string, offset int64, pageSize optional.Int64) ([]hosts.Host, int64, bool, error) {
	ps := int64(-1)
	if pageSize.Valid {
		ps = pageSize.Int64
	}

	return b.listHosts(
		ctx,
		querySelectClusterHosts,
		ListHostsArgs{
			ClusterID:    clusterID,
			SubClusterID: -1,
			Role:         hosts.RoleUnknown,
			Revision:     -1,
			PageSize:     ps,
			Offset:       offset,
		},
	)
}

func (b *Backend) HostsByClusterIDRoleAtRevision(ctx context.Context, clusterID string, role hosts.Role, rev int64) ([]hosts.Host, error) {
	res, _, _, err := b.listHosts(
		ctx,
		querySelectClusterHostsAtRevision,
		ListHostsArgs{
			ClusterID:    clusterID,
			SubClusterID: -1,
			Role:         role,
			Revision:     rev,
			PageSize:     -1,
			Offset:       -1,
		},
	)
	return res, err
}

func (b *Backend) HostsByClusterIDShardNameAtRevision(ctx context.Context, clusterID string, shardName string, rev int64) ([]hosts.Host, error) {
	res, _, _, err := b.listHosts(
		ctx,
		querySelectClusterHostsAtRevision,
		ListHostsArgs{
			ClusterID:    clusterID,
			SubClusterID: -1,
			ShardName:    shardName,
			Revision:     rev,
			PageSize:     -1,
			Offset:       -1,
		},
	)
	return res, err
}

func (b *Backend) HostsByShardID(ctx context.Context, shardID, clusterID string) ([]hosts.Host, error) {
	var res []hosts.Host
	parser := func(rows *sqlx.Rows) error {
		var h host
		if err := rows.StructScan(&h); err != nil {
			return err
		}

		host, err := hostFromDB(h, clusterID)
		if err != nil {
			return err
		}

		res = append(res, host)
		return nil
	}

	_, err := sqlutil.QueryTx(
		ctx,
		queryGetHostsByShard,
		map[string]interface{}{
			"shard_id": shardID,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return nil, err
	}

	return res, nil
}

func (b *Backend) DeleteHosts(ctx context.Context, clusterID string, fqdns []string, revision int64) ([]hosts.Host, error) {
	var dbHosts []host
	parser := func(rows *sqlx.Rows) error {
		var h host
		if err := rows.StructScan(&h); err != nil {
			return err
		}

		dbHosts = append(dbHosts, h)
		return nil
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryDeleteHosts,
		map[string]interface{}{
			"cid":   clusterID,
			"rev":   revision,
			"fqdns": fqdns,
		},
		parser,
		b.logger,
	)

	if err != nil {
		return nil, err
	}
	if count == 0 {
		return nil, sqlerrors.ErrNotFound
	}
	var res []hosts.Host
	for _, h := range dbHosts {
		host, err := hostFromDB(h, clusterID)
		if err != nil {
			return nil, err
		}
		res = append(res, host)
	}
	return res, err
}

//noinspection ALL
type host struct {
	SubClusterID        string           `db:"subcid"`
	ShardID             sql.NullString   `db:"shard_id"`
	ShardName           sql.NullString   `db:"shard_name"`
	ResourcePresetID    uuid.UUID        `db:"flavor_id"`
	ResourcePresetExtID string           `db:"flavor_name"`
	SpaceLimit          int64            `db:"space_limit"`
	ZoneID              string           `db:"geo"`
	FQDN                string           `db:"fqdn"`
	DiskTypeExtID       string           `db:"disk_type_id"`
	SubnetID            string           `db:"subnet_id"`
	AssignPublicIP      bool             `db:"assign_public_ip"`
	VType               string           `db:"vtype"`
	VTypeID             sql.NullString   `db:"vtype_id"`
	Revision            int              `db:"rev"`
	CreatedAt           time.Time        `db:"created_at"`
	Roles               pgtype.TextArray `db:"roles"`
	ReplicaPriority     optional.Int64   `db:"replica_priority"`
}

func hostFromDB(h host, cid string) (hosts.Host, error) {
	v := hosts.Host{
		ClusterID:           cid,
		SubClusterID:        h.SubClusterID,
		ShardID:             optional.String(h.ShardID),
		SpaceLimit:          h.SpaceLimit,
		ResourcePresetID:    resources.PresetID(h.ResourcePresetID.UUID),
		ResourcePresetExtID: h.ResourcePresetExtID,
		ZoneID:              h.ZoneID,
		FQDN:                h.FQDN,
		DiskTypeExtID:       h.DiskTypeExtID,
		SubnetID:            h.SubnetID,
		AssignPublicIP:      h.AssignPublicIP,
		Roles:               []hosts.Role{},
		VType:               h.VType,
		VTypeID:             h.VTypeID,
		ReplicaPriority:     h.ReplicaPriority,
	}
	for _, t := range h.Roles.Elements {
		role, err := hosts.ParseRole(t.String)
		if err != nil {
			return hosts.Host{}, err
		}
		v.Roles = append(v.Roles, role)
	}
	return v, nil
}
