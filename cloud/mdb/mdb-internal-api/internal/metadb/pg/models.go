package pg

import (
	"database/sql"
	"encoding/json"
	"reflect"
	"strings"
	"time"

	"github.com/jackc/pgtype"
	uuid "github.com/jackc/pgtype/ext/gofrs-uuid"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type operation struct {
	OperationID         string         `db:"operation_id"`
	TargetID            string         `db:"target_id"`
	ClusterID           string         `db:"cid"`
	ClusterType         string         `db:"cluster_type"`
	Environment         string         `db:"env"`
	Type                string         `db:"operation_type"`
	CreatedBy           string         `db:"created_by"`
	CreatedAt           time.Time      `db:"created_at"`
	StartedAt           sql.NullTime   `db:"started_at"`
	ModifiedAt          time.Time      `db:"modified_at"`
	Status              string         `db:"status"`
	MetaData            pgtype.JSON    `db:"metadata"`
	Args                pgtype.JSON    `db:"args"`
	Hidden              bool           `db:"hidden"`
	RequiredOperationID sql.NullString `db:"required_operation_id"`
	Errors              pgtype.JSON    `db:"errors"`
}

func operationFromDB(op operation) (operations.Operation, error) {
	res := operations.Operation{
		OperationID: op.OperationID,
		TargetID:    op.TargetID,
		ClusterID:   op.ClusterID,
		Environment: op.Environment,
		CreatedBy:   op.CreatedBy,
		CreatedAt:   op.CreatedAt,
		ModifiedAt:  op.ModifiedAt,
		Hidden:      op.Hidden,
	}

	if op.StartedAt.Valid {
		res.StartedAt = op.StartedAt.Time
	}

	if op.RequiredOperationID.Valid {
		res.RequiredOperationID = op.RequiredOperationID.String
	}

	cType, err := clusters.ParseTypeStringified(op.ClusterType)
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("operation %q cluster type: %w", res.OperationID, err)
	}
	res.ClusterType = cType

	ot, err := operations.ParseType(op.Type)
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("operation %q type: %w", res.OperationID, err)
	}
	res.Type = ot

	status, err := operations.ParseStatus(op.Status)
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("operation %q status: %w", res.OperationID, err)
	}
	res.Status = status

	if err = op.Args.AssignTo(&res.Args); err != nil {
		return operations.Operation{}, xerrors.Errorf("operation %q arguments %+v: %w", res.OperationID, op.Args, err)
	}

	if err = op.Errors.AssignTo(&res.Errors); err != nil {
		// https://st.yandex-team.ru/MDB-11155
		if string(op.Errors.Bytes) == "{}" {
			res.Errors = nil
		} else {
			return operations.Operation{}, xerrors.Errorf("operation %q errors %+v: %w", res.OperationID, op.Errors, err)
		}
	}

	desc, err := operations.GetDescriptor(res.Type)
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("operation %q metadata: %w", res.OperationID, err)
	}

	if desc.MetadataType != nil {
		// Create new value of specified type (its a pointer)
		v := reflect.New(desc.MetadataType)
		// Unmarshal JSON to created value
		if err = op.MetaData.AssignTo(v.Interface()); err != nil {
			// TODO: non-fatal - report to sentry and continue
			return operations.Operation{}, xerrors.Errorf("operation %q metadata %+v: %w", res.OperationID, op.MetaData, err)
		}

		// Retrieve value itself and store it in metadata
		res.MetaData = reflect.Indirect(v).Interface()
	}

	return res, nil
}

type cluster struct {
	ClusterID          string           `db:"cid"`
	Name               string           `db:"name"`
	Type               string           `db:"type"`
	Environment        string           `db:"env"`
	CreatedAt          time.Time        `db:"created_at"`
	NetworkID          string           `db:"network_id"`
	UserSGroupIDs      pgtype.TextArray `db:"user_sgroup_ids"`
	HostGroupIDs       pgtype.TextArray `db:"host_group_ids"`
	Status             string           `db:"status"`
	Pillar             json.RawMessage  `db:"pillar"`
	Description        sql.NullString   `db:"description"`
	Labels             json.RawMessage  `db:"labels"`
	Revision           int64            `db:"rev"`
	BackupSchedule     json.RawMessage  `db:"backup_schedule"`
	DeletionProtection bool             `db:"deletion_protection"`
}

type subCluster struct {
	ClusterID    string           `db:"cid"`
	SubClusterID string           `db:"subcid"`
	Name         string           `db:"name"`
	Roles        pgtype.TextArray `db:"roles"`
	Pillar       json.RawMessage  `db:"pillar"`
	CreatedAt    time.Time        `db:"created_at"`
}

type kubernetesNodeGroup struct {
	KubernetesClusterID sql.NullString `db:"kubernetes_cluster_id"`
	NodeGroupID         sql.NullString `db:"node_group_id"`
	SubClusterID        string         `db:"subcid"`
}

type resourcePreset struct {
	ID               uuid.UUID `db:"id"`
	CPUGuarantee     float64   `db:"cpu_guarantee"`
	CPULimit         float64   `db:"cpu_limit"`
	CPUFraction      float64   `db:"cpu_fraction"`
	IOCoresLimit     int64     `db:"io_cores_limit"`
	GPULimit         int64     `db:"gpu_limit"`
	MemoryGuarantee  int64     `db:"memory_guarantee"`
	MemoryLimit      int64     `db:"memory_limit"`
	NetworkGuarantee int64     `db:"network_guarantee"`
	NetworkLimit     int64     `db:"network_limit"`
	IOLimit          int64     `db:"io_limit"`
	ExtID            string    `db:"name"`
	Description      string    `db:"description"`
	VType            string    `db:"vtype"`
	Type             string    `db:"type"`
	Generation       int64     `db:"generation"`
	PlatformID       string    `db:"platform_id"`
}

type diskType struct {
	DiskTypeExtID string `db:"disk_type_ext_id"`
	QuotaType     string `db:"quota_type"`
}

type shard struct {
	SubClusterID string          `db:"subcid"`
	ShardID      string          `db:"shard_id"`
	Name         string          `db:"name"`
	CreatedAt    time.Time       `db:"created_at"`
	Pillar       json.RawMessage `db:"pillar"`
}

func clusterFromDB(c cluster) (metadb.Cluster, error) {
	env, err := environment.ParseSaltEnv(c.Environment)
	if err != nil {
		return metadb.Cluster{}, err
	}

	status, err := clusters.ParseStatus(c.Status)
	if err != nil {
		return metadb.Cluster{}, err
	}

	res := metadb.Cluster{
		Cluster: clusters.Cluster{
			ClusterID:          c.ClusterID,
			Name:               c.Name,
			Environment:        env,
			CreatedAt:          c.CreatedAt,
			NetworkID:          c.NetworkID,
			Status:             status,
			Description:        c.Description.String,
			Labels:             make(map[string]string),
			Revision:           c.Revision,
			SecurityGroupIDs:   make([]string, 0, len(c.UserSGroupIDs.Elements)),
			HostGroupIDs:       make([]string, 0, len(c.HostGroupIDs.Elements)),
			DeletionProtection: c.DeletionProtection,
		},
		Pillar: c.Pillar,
	}

	if len(c.BackupSchedule) > 0 {
		if err := json.Unmarshal(c.BackupSchedule, &res.BackupSchedule); err != nil {
			return metadb.Cluster{}, xerrors.Errorf("failed to unmarshall backup_schedule %q: %w", string(c.BackupSchedule), err)
		}
	}

	if res.Type, err = clusters.ParseTypeStringified(c.Type); err != nil {
		return metadb.Cluster{}, err
	}

	for _, sg := range c.UserSGroupIDs.Elements {
		res.SecurityGroupIDs = append(res.SecurityGroupIDs, sg.String)
	}

	for _, hg := range c.HostGroupIDs.Elements {
		res.HostGroupIDs = append(res.HostGroupIDs, hg.String)
	}

	if c.Labels != nil {
		if err = json.Unmarshal(c.Labels, &res.Labels); err != nil {
			return metadb.Cluster{}, xerrors.Errorf("failed to unmarshal labels from metadb: %w", err)
		}
	}

	return res, nil
}

func nodeGroupFromDB(ng kubernetesNodeGroup) (metadb.KubernetesNodeGroup, error) {
	nodeGroup := metadb.KubernetesNodeGroup{
		SubClusterID:        ng.SubClusterID,
		KubernetesClusterID: ng.KubernetesClusterID.String,
		NodeGroupID:         ng.NodeGroupID.String,
	}
	return nodeGroup, nil
}

func subClusterFromDB(s subCluster) (metadb.SubCluster, error) {
	sc := metadb.SubCluster{
		SubCluster: clusters.SubCluster{
			SubClusterID: s.SubClusterID,
			ClusterID:    s.ClusterID,
			Name:         s.Name,
		},
		Pillar: s.Pillar,
	}

	for _, t := range s.Roles.Elements {
		role, err := hosts.ParseRole(t.String)
		if err != nil {
			return metadb.SubCluster{}, err
		}
		sc.Roles = append(sc.Roles, role)
	}

	return sc, nil
}

func resourcePresetFromDB(rs resourcePreset) (resources.Preset, error) {
	vType, err := environment.ParseVType(rs.VType)
	if err != nil {
		return resources.Preset{}, err
	}
	r := resources.Preset{
		ID:               resources.PresetID(rs.ID.UUID),
		CPUGuarantee:     rs.CPUGuarantee,
		CPULimit:         rs.CPULimit,
		CPUFraction:      rs.CPUFraction,
		IOCoresLimit:     rs.IOCoresLimit,
		GPULimit:         rs.GPULimit,
		MemoryGuarantee:  rs.MemoryGuarantee,
		MemoryLimit:      rs.MemoryLimit,
		NetworkGuarantee: rs.NetworkGuarantee,
		NetworkLimit:     rs.NetworkLimit,
		IOLimit:          rs.IOLimit,
		ExtID:            rs.ExtID,
		Description:      rs.Description,
		VType:            vType,
		Type:             rs.Type,
		Generation:       rs.Generation,
		PlatformID:       rs.PlatformID,
	}

	return r, nil
}

func shardFromDB(s shard) (metadb.Shard, error) {
	return metadb.Shard{
		Shard: clusters.Shard{
			SubClusterID: s.SubClusterID,
			ShardID:      s.ShardID,
			Name:         s.Name,
			CreatedAt:    s.CreatedAt,
		},
		Pillar: s.Pillar,
	}, nil
}

var diskTypesFromDBMapping = map[string]resources.DiskType{
	"ssd": resources.DiskTypeSSD,
	"hdd": resources.DiskTypeHDD,
}

func diskTypeFromDB(s string) (resources.DiskType, error) {
	dt, ok := diskTypesFromDBMapping[strings.ToLower(s)]
	if !ok {
		return resources.DiskTypeUnknown, xerrors.Errorf("unknown disk type: %s", s)
	}

	return dt, nil
}

type defaultVersion struct {
	MajorVersion string           `db:"major_version"`
	MinorVersion string           `db:"minor_version"`
	Name         string           `db:"name"`
	Edition      string           `db:"edition"`
	IsDefault    bool             `db:"is_default"`
	IsDeprecated bool             `db:"is_deprecated"`
	UpdatableTo  pgtype.TextArray `db:"updatable_to"`
}

func defaultVersionFromDB(v defaultVersion) (console.DefaultVersion, error) {
	var upTo []string

	if v.UpdatableTo.Status != pgtype.Null {
		for _, e := range v.UpdatableTo.Elements {
			if e.Status != pgtype.Null {
				upTo = append(upTo, e.String)
			}
		}
	}

	return console.DefaultVersion{
		MajorVersion: v.MajorVersion,
		MinorVersion: v.MinorVersion,
		Name:         v.Name,
		Edition:      v.Edition,
		IsDefault:    v.IsDefault,
		IsDeprecated: v.IsDeprecated,
		UpdatableTo:  upTo,
	}, nil
}

type version struct {
	ClusterID      sql.NullString `db:"cid"`
	SubClusterID   sql.NullString `db:"subcid"`
	ShardID        sql.NullString `db:"shard_id"`
	Component      string         `db:"component"`
	MajorVersion   string         `db:"major_version"`
	MinorVersion   string         `db:"minor_version"`
	PackageVersion string         `db:"package_version"`
	Edition        string         `db:"edition"`
}

func versionFromDB(v version) (console.Version, error) {
	return console.Version{
		ClusterID:      optional.String{String: v.ClusterID.String, Valid: v.ClusterID.Valid},
		SubClusterID:   optional.String{String: v.SubClusterID.String, Valid: v.SubClusterID.Valid},
		ShardID:        optional.String{String: v.ShardID.String, Valid: v.ShardID.Valid},
		Component:      v.Component,
		MajorVersion:   v.MajorVersion,
		MinorVersion:   v.MinorVersion,
		PackageVersion: v.PackageVersion,
		Edition:        v.Edition,
	}, nil
}

type quotaUsage struct {
	CPU      float64 `db:"cpu"`
	Memory   int64   `db:"memory"`
	SSDSpace int64   `db:"ssd_space"`
	HDDSpace int64   `db:"hdd_space"`
	Clusters int64   `db:"clusters"`
}

func quotaUsageFromDB(qu quotaUsage) (metadb.Resources, error) {
	return metadb.Resources{
		CPU:      qu.CPU,
		Memory:   qu.Memory,
		SSDSpace: qu.SSDSpace,
		HDDSpace: qu.HDDSpace,
		Clusters: qu.Clusters,
	}, nil
}

type ListHostsArgs struct {
	ClusterID    string
	SubClusterID int64
	Role         hosts.Role
	ShardName    string
	Revision     int64
	PageSize     int64
	Offset       int64
}

func (lha ListHostsArgs) toMap() map[string]interface{} {
	res := map[string]interface{}{
		"cid":        nil,
		"subcid":     nil,
		"role":       nil,
		"shard_name": nil,
		"revision":   nil,
		"limit":      nil,
		"offset":     nil,
	}

	if lha.ClusterID != "" {
		res["cid"] = lha.ClusterID
	}
	if lha.SubClusterID >= 0 {
		res["subcid"] = lha.SubClusterID
	}
	if lha.Role != hosts.RoleUnknown {
		res["role"] = lha.Role.Stringified()
	}
	if lha.ShardName != "" {
		res["shard_name"] = lha.ShardName
	}
	if lha.Revision >= 0 {
		res["revision"] = lha.Revision
	}
	if lha.PageSize > 0 {
		res["limit"] = lha.PageSize
	}
	if lha.Offset >= 0 {
		res["offset"] = lha.Offset
	}

	return res
}

type maintenanceInfo struct {
	ClusterID    string           `db:"cid"`
	Day          pgtype.Text      `db:"day"`
	Hour         pgtype.Int8      `db:"hour"`
	ConfigID     pgtype.Text      `db:"config_id"`
	CreatedAt    pgtype.Timestamp `db:"created_at"`
	DelayedUntil pgtype.Timestamp `db:"delayed_until"`
	Info         pgtype.Text      `db:"info"`
}

func (mi maintenanceInfo) ValidWindow() bool {
	return mi.Day.Status != pgtype.Null && mi.Hour.Status != pgtype.Null
}

func (mi maintenanceInfo) ValidOperation() bool {
	return mi.ConfigID.Status != pgtype.Null &&
		mi.CreatedAt.Status != pgtype.Null &&
		mi.DelayedUntil.Status != pgtype.Null &&
		mi.Info.Status != pgtype.Null
}

func maintenanceInfoFromDB(info maintenanceInfo, found bool) clusters.MaintenanceInfo {
	if !found {
		return clusters.MaintenanceInfo{
			Window:    clusters.NewAnytimeMaintenanceWindow(),
			Operation: clusters.MaintenanceOperation{},
		}
	}

	var (
		maintenanceWindow    clusters.MaintenanceWindow
		maintenanceOperation clusters.MaintenanceOperation
	)

	if info.ValidWindow() {
		maintenanceWindow = clusters.NewWeeklyMaintenanceWindow(
			info.Day.String,
			int(info.Hour.Int),
		)
	} else {
		maintenanceWindow = clusters.NewAnytimeMaintenanceWindow()
	}

	if info.ValidOperation() {
		maintenanceOperation = clusters.NewMaintenanceOperation(
			info.ConfigID.String,
			info.CreatedAt.Time,
			info.DelayedUntil.Time,
			info.Info.String,
		)
	}

	return clusters.MaintenanceInfo{
		ClusterID: info.ClusterID,
		Window:    maintenanceWindow,
		Operation: maintenanceOperation,
	}
}
