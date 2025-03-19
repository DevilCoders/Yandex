package models

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
)

type Visibility int

const (
	VisibilityAll Visibility = iota
	VisibilityVisibleOrDeleted
	VisibilityVisible
)

type CreateClusterArgs struct {
	ClusterID          string
	Name               string
	ClusterType        clusters.Type
	Environment        environment.SaltEnv
	PublicKey          []byte
	NetworkID          string
	FolderID           int64
	Pillar             json.RawMessage
	Description        string
	Labels             clusters.Labels
	HostGroupIDs       []string
	DeletionProtection bool
	MaintenanceWindow  clusters.MaintenanceWindow
}

func (args CreateClusterArgs) Validate() error {
	if err := args.Labels.Validate(); err != nil {
		return err
	}

	if err := args.MaintenanceWindow.Validate(); err != nil {
		return err
	}

	return nil
}

type ListClusterArgs struct {
	ClusterID     optional.String
	Name          optional.String
	ClusterType   clusters.Type
	Environment   environment.SaltEnv
	FolderID      int64
	Limit         optional.Int64
	Offset        int64
	PageTokenName optional.String
	Visibility    Visibility
}

type ListClusterIDsArgs struct {
	ClusterType    clusters.Type
	Visibility     Visibility
	Limit          int64
	AfterClusterID optional.String
}

type CreateSubClusterArgs struct {
	ClusterID    string
	SubClusterID string
	Name         string
	Roles        []hosts.Role
	Revision     int64
}

type DeleteSubClusterArgs struct {
	ClusterID    string
	SubClusterID string
	Revision     int64
}

type CreateShardArgs struct {
	ClusterID    string
	SubClusterID string
	ShardID      string
	Name         string
	Revision     int64
}

type AddHostArgs struct {
	SubClusterID     string
	ShardID          optional.String
	ResourcePresetID resources.PresetID
	SpaceLimit       int64
	ZoneID           string
	FQDN             string
	DiskTypeExtID    string
	SubnetID         string
	AssignPublicIP   bool
	ClusterID        string
	Revision         int64
}

type ModifyHostArgs struct {
	FQDN             string
	ClusterID        string
	Revision         int64
	SpaceLimit       int64
	ResourcePresetID resources.PresetID
	DiskTypeExtID    string
}

type ClusterResources struct {
	ResourcePresetExtID string
	DiskSize            int64
	DiskTypeExtID       string
}

type AddDiskPlacementGroupArgs struct {
	ClusterID string
	LocalID   int
	Revision  int64
}

type AddDiskArgs struct {
	ClusterID  string
	LocalID    int
	FQDN       string
	MountPoint string
	Revision   int64
}

type AddPlacementGroupArgs struct {
	ClusterID string
	FQDN      string
	LocalID   int
	Revision  int64
}

func (cr ClusterResources) Validate() error {
	if cr.ResourcePresetExtID == "" {
		return semerr.InvalidInput("resource preset must be specified")
	}
	if cr.DiskTypeExtID == "" {
		return semerr.InvalidInput("disk type must be specified")
	}
	if cr.DiskSize < 0 {
		return semerr.InvalidInputf("disk size must be a positive integer value (%d)", cr.DiskSize)
	}
	return nil
}

func (cr *ClusterResources) Merge(spec ClusterResourcesSpec) {
	if spec.DiskSize.Valid {
		cr.DiskSize = spec.DiskSize.Int64
	}
	if spec.DiskTypeExtID.Valid {
		cr.DiskTypeExtID = spec.DiskTypeExtID.String
	}
	if spec.ResourcePresetExtID.Valid {
		cr.ResourcePresetExtID = spec.ResourcePresetExtID.String
	}
}

type ClusterResourcesSpec struct {
	ResourcePresetExtID optional.String
	DiskSize            optional.Int64
	DiskTypeExtID       optional.String
}

func (crs *ClusterResourcesSpec) IsSet() bool {
	return crs.ResourcePresetExtID.Valid || crs.DiskTypeExtID.Valid || crs.DiskSize.Valid
}

func (crs *ClusterResourcesSpec) MustOptionals() ClusterResources {
	return ClusterResources{
		ResourcePresetExtID: crs.ResourcePresetExtID.Must(),
		DiskSize:            crs.DiskSize.Must(),
		DiskTypeExtID:       crs.DiskTypeExtID.Must(),
	}
}

func (crs *ClusterResourcesSpec) Validate(allRequired bool) error {
	if allRequired || crs.ResourcePresetExtID.Valid {
		if !crs.ResourcePresetExtID.Valid || crs.ResourcePresetExtID.String == "" {
			return semerr.InvalidInput("resource preset must be specified")
		}
	}
	if allRequired || crs.DiskTypeExtID.Valid {
		if !crs.DiskTypeExtID.Valid || crs.DiskTypeExtID.String == "" {
			return semerr.InvalidInput("disk type must be specified")
		}
	}
	if allRequired || crs.DiskSize.Valid {
		if !crs.DiskSize.Valid || crs.DiskSize.Int64 == 0 {
			return semerr.InvalidInput("disk size must be specified")
		}
		if crs.DiskSize.Int64 < 0 {
			return semerr.InvalidInputf("disk size must be a positive integer value (%d)", crs.DiskSize.Int64)
		}
	}
	return nil
}
