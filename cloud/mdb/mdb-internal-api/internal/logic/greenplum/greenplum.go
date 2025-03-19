package greenplum

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum/gpmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	omodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
)

//go:generate ../../../../scripts/mockgen.sh Greenplum

const OneSegmentMemLimit int64 = 8 * 1024 * 1024 * 1024
const LowMemMultiplicator int64 = 10

type Greenplum interface {
	CreateCluster(ctx context.Context, args CreateClusterArgs) (operations.Operation, error)
	RestoreCluster(ctx context.Context, args CreateClusterArgs, extBackupID string) (operations.Operation, error)
	Cluster(ctx context.Context, cid string) (gpmodels.Cluster, error)
	Clusters(ctx context.Context, folderExtID string, limit, offset int64) ([]gpmodels.Cluster, error)
	DeleteCluster(ctx context.Context, cid string) (operations.Operation, error)

	ModifyCluster(ctx context.Context, args ModifyClusterArgs) (operations.Operation, error)
	AddHosts(ctx context.Context, args AddHostsClusterArgs) (operations.Operation, error)

	ListMasterHosts(ctx context.Context, cid string) ([]hosts.HostExtended, error)
	ListSegmentHosts(ctx context.Context, cid string) ([]hosts.HostExtended, error)

	StartCluster(ctx context.Context, cid string) (operations.Operation, error)
	StopCluster(ctx context.Context, cid string) (operations.Operation, error)

	EstimateCreateCluster(ctx context.Context, args CreateClusterArgs) (console.BillingEstimate, error)

	GetDefaultVersions(ctx context.Context) ([]console.DefaultVersion, error)
	IsLowMemSegmentAllowed(ctx context.Context, folderExtID string) (bool, error)
	GetHostGroupType(ctx context.Context, hostGroupIds []string) (map[string]compute.HostGroupHostType, error)
	RescheduleMaintenance(ctx context.Context, cid string, rescheduleType clusters.RescheduleType, delayedUntil optional.Time) (operations.Operation, error)

	Backup(ctx context.Context, backupID string) (bmodels.Backup, error)
	FolderBackups(ctx context.Context, fid string, pageToken bmodels.BackupsPageToken, pageSize int64) ([]bmodels.Backup, bmodels.BackupsPageToken, error)
	ClusterBackups(ctx context.Context, cid string, pageToken bmodels.BackupsPageToken, pageSize int64) ([]bmodels.Backup, bmodels.BackupsPageToken, error)
	RestoreHints(ctx context.Context, globalBackupID string) (gpmodels.RestoreHints, error)
	GetRecommendedConfig(ctx context.Context, args *RecommendedConfigArgs) (*RecommendedConfig, error)
}

type CreateClusterArgs struct {
	FolderExtID string
	Name        string
	Description string
	Labels      map[string]string
	Environment environment.SaltEnv

	Config           gpmodels.ClusterConfig
	MasterConfig     gpmodels.MasterSubclusterConfigSpec
	SegmentConfig    gpmodels.SegmentSubclusterConfigSpec
	ConfigSpec       gpmodels.ConfigSpec
	MasterHostCount  int
	SegmentHostCount int
	SegmentInHost    int

	UserName     string
	UserPassword string

	NetworkID          string
	SecurityGroupIDs   []string
	DeletionProtection bool
	HostGroupIDs       []string
	MaintenanceWindow  clusters.MaintenanceWindow
}

func (args *CreateClusterArgs) ValidateRestore(sourceMasterResources, sourceSegmentResources models.ClusterResources) error {
	if args.MasterConfig.Resources.DiskSize < sourceMasterResources.DiskSize {
		return semerr.FailedPreconditionf("insufficient diskSize of master hosts, increase it to '%d'", sourceMasterResources.DiskSize)
	}

	if args.SegmentConfig.Resources.DiskSize < sourceSegmentResources.DiskSize {
		return semerr.FailedPreconditionf("insufficient diskSize of segment hosts, increase it to '%d'", sourceSegmentResources.DiskSize)
	}
	return nil
}

func (args *CreateClusterArgs) GetValidatedVersion(versions []console.DefaultVersion, ignoreDeprecated bool) (console.DefaultVersion, error) {
	for _, version := range versions {
		if version.MajorVersion == args.Config.Version {
			if version.IsDeprecated && !ignoreDeprecated {
				return console.DefaultVersion{}, semerr.InvalidInputf("unsupported version: %s", args.Config.Version)
			}
			return version, nil
		}
		if args.Config.Version == "" && version.IsDefault {
			return version, nil
		}
	}
	return console.DefaultVersion{}, semerr.InvalidInputf("unknown version: %s", args.Config.Version)
}

var SystemUserNames = []string{
	"public",
	"postgres",
	"gpadmin",
	"admin",
	"repl",
	"monitor",
	"none",
	"mdb_admin",
	"mdb_replication",
}
var UserNameValidator = models.MustUserNameValidator(models.DefaultUserNamePattern, SystemUserNames)
var userPasswordValidator = models.MustUserPasswordValidator(models.DefaultUserPasswordPattern)

func (args *CreateClusterArgs) Validate(segmentPreset resources.Preset, masterPreset resources.Preset, allowLowMem, ignoreUserValidation bool, hostGroupHostType map[string]compute.HostGroupHostType) error {
	if err := models.ClusterNameValidator.ValidateString(args.Name); err != nil {
		return err
	}
	if err := models.ClusterDescriptionValidator.ValidateString(args.Description); err != nil {
		return err
	}

	if args.MasterHostCount <= 0 {
		return semerr.InvalidInput("master host count must be 1 or 2")
	}
	if args.MasterHostCount > 2 {
		return semerr.InvalidInput("master host count must be 1 or 2")
	}

	if args.SegmentHostCount < 2 {
		return semerr.InvalidInput("segment host count must be 2 or more")
	}

	if args.SegmentInHost <= 0 {
		return semerr.InvalidInput("segment in host must be 1 or more")
	}

	if err := UserNameValidator.ValidateString(args.UserName); err != nil && !ignoreUserValidation {
		return err
	}
	if err := userPasswordValidator.ValidateString(args.UserPassword); err != nil && !ignoreUserValidation {
		return err
	}

	if (args.MasterConfig.Resources.DiskTypeExtID == resources.NetworkSSDNonreplicated ||
		args.MasterConfig.Resources.DiskTypeExtID == resources.LocalSSD ||
		args.MasterConfig.Resources.DiskTypeExtID == resources.LocalHDD) &&
		args.MasterHostCount != 2 {
		return semerr.InvalidInputf("master host count must be 2 for %s disk", args.MasterConfig.Resources.DiskTypeExtID)
	}

	allowedSegmentInHost := MaxSegmentInHostCountCalc(segmentPreset.MemoryLimit, allowLowMem)
	if args.SegmentInHost > int(allowedSegmentInHost) {
		return semerr.InvalidInputf("the segment in host must be no more then ram/8gb: %d; RAM: %d", allowedSegmentInHost, segmentPreset.MemoryLimit)
	}

	for _, hght := range hostGroupHostType {
		generation := hght.Generation()
		if segmentPreset.Generation != generation {
			return semerr.InvalidInputf("resource preset %q and dedicated hosts group %q have different platforms", args.SegmentConfig.Resources.ResourcePresetExtID, hght.HostGroup.ID)
		}
		if masterPreset.Generation != generation {
			return semerr.InvalidInputf("resource preset %q and dedicated hosts group %q have different platforms", args.MasterConfig.Resources.ResourcePresetExtID, hght.HostGroup.ID)
		}
	}

	if len(hostGroupHostType) > 0 {
		minCores, minMemory, minDisks, diskSize, diskSizeHostGroup := compute.GetMinHostGroupResources(hostGroupHostType, args.Config.ZoneID)
		if segmentPreset.MemoryLimit > minMemory {
			return semerr.InvalidInputf("cannot use resource preset %q on dedicated host, max memory is %d", args.SegmentConfig.Resources.ResourcePresetExtID, minMemory)
		}
		if segmentPreset.CPULimit > float64(minCores) {
			return semerr.InvalidInputf("cannot use resource preset %q on dedicated host, max cpu limit is %d", args.SegmentConfig.Resources.ResourcePresetExtID, minCores)
		}
		if masterPreset.MemoryLimit > minMemory {
			return semerr.InvalidInputf("cannot use resource preset %q on dedicated host, max memory is %d", args.MasterConfig.Resources.ResourcePresetExtID, minMemory)
		}
		if masterPreset.CPULimit > float64(minCores) {
			return semerr.InvalidInputf("cannot use resource preset %q on dedicated host, max cpu limit is %d", args.MasterConfig.Resources.ResourcePresetExtID, minCores)
		}
		if args.MasterConfig.Resources.DiskTypeExtID == resources.LocalSSD || args.SegmentConfig.Resources.DiskTypeExtID == resources.LocalSSD {
			var uniqHostType int64
			for k := range diskSizeHostGroup {
				if k > 0 {
					uniqHostType++
				}
			}
			if uniqHostType > 1 {
				return semerr.InvalidInputf("different disk_size in host groups is not allowed")
			}
			maxDiskSize := minDisks * diskSize
			if args.MasterConfig.Resources.DiskTypeExtID == resources.LocalSSD {
				if maxDiskSize < args.MasterConfig.Resources.DiskSize {
					return semerr.InvalidInputf("cannot use disk size %d on host group, max disk size is %d", args.MasterConfig.Resources.DiskSize, maxDiskSize)
				}
				if args.MasterConfig.Resources.DiskSize%diskSize != 0 {
					return semerr.InvalidInputf("on host group disk size should by multiple of %d", diskSize)
				}
			}
			if args.SegmentConfig.Resources.DiskTypeExtID == resources.LocalSSD {
				if maxDiskSize < args.SegmentConfig.Resources.DiskSize {
					return semerr.InvalidInputf("cannot use disk size %d on host group, max disk size is %d", args.SegmentConfig.Resources.DiskSize, maxDiskSize)
				}
				if args.SegmentConfig.Resources.DiskSize%diskSize != 0 {
					return semerr.InvalidInputf("on host group disk size should by multiple %d", diskSize)
				}
			}

		}
	}

	// TODO: others checks
	return nil
}

func MaxSegmentInHostCountCalc(memAmount int64, allowLowMem bool) int64 {
	if allowLowMem {
		return (memAmount * LowMemMultiplicator) / OneSegmentMemLimit
	}
	if memAmount < OneSegmentMemLimit*2 {
		return 1
	}
	if memAmount/OneSegmentMemLimit >= 16 {
		return 16
	} else {
		return memAmount / OneSegmentMemLimit
	}
}

type OptInt struct {
	IsSet bool
	Value *int64
}

func (o *OptInt) Get(defaultValue *int64, pillarChanges bool) (*int64, bool) {
	if o.IsSet {
		return o.Value, true
	}
	return defaultValue, pillarChanges
}

type OptString struct {
	IsSet bool
	Value *string
}

func (o *OptString) Get(defaultValue *string, pillarChanges bool) (*string, bool) {
	if o.IsSet {
		return o.Value, true
	}
	return defaultValue, pillarChanges
}

type OptBool struct {
	IsSet bool
	Value *bool
}

func (o *OptBool) Get(defaultValue *bool, pillarChanges bool) (*bool, bool) {
	if o.IsSet {
		return o.Value, true
	}
	return defaultValue, pillarChanges
}

type OptFloat struct {
	IsSet bool
	Value *float32
}

func (o *OptFloat) Get(defaultValue *float32, pillarChanges bool) (*float32, bool) {
	if o.IsSet {
		return o.Value, true
	}
	return defaultValue, pillarChanges
}

type ModifyClusterArgs struct {
	ClusterID          string
	SecurityGroupsIDs  optional.Strings
	Name               optional.String
	Description        optional.String
	Labels             omodels.Labels
	DeletionProtection optional.Bool

	BackupWindowStart bmodels.OptionalBackupWindowStart
	MaintenanceWindow omodels.MaintenanceWindow

	UserPassword optional.String

	Access struct {
		WebSQL       optional.Bool
		DataLens     optional.Bool
		DataTransfer optional.Bool
		Serverless   optional.Bool
	}

	MasterConfig struct {
		Resource struct {
			ResourcePresetExtID OptString
			DiskSize            OptInt
			DiskTypeExtID       OptString
		}
	}

	SegmentConfig struct {
		Resource struct {
			ResourcePresetExtID OptString
			DiskSize            OptInt
			DiskTypeExtID       OptString
		}
	}
	ConfigSpec gpmodels.ConfigSpec
}

type AddHostsClusterArgs struct {
	ClusterID        string
	MasterHostCount  int64
	SegmentHostCount int64
}
type RecommendedConfigArgs struct {
	DataSize          int64
	DiskTypeID        string
	UseDedicatedHosts bool
	FlavorType        string
	FolderID          string
}
type RecommendedConfig struct {
	MasterHosts       int64
	SegmentHosts      int64
	UseDedicatedHosts bool
	MasterConfig      struct {
		Resource struct {
			ResourcePresetExtID string
			DiskSize            int64
			DiskTypeExtID       string
			Type                string
			Generation          int64
		}
	}
	SegmentConfig struct {
		Resource struct {
			ResourcePresetExtID string
			DiskSize            int64
			DiskTypeExtID       string
			Type                string
			Generation          int64
		}
	}
	SegmentsPerHost int64
}

type CalculatedResources struct {
	CPU       int64
	MEM       int64
	DISK      int64
	Dedicated bool
}
type RecommendedRatios struct {
	CompressionRatio      int64
	CloudPerformanceRatio int64
	UnknownRatio          float64

	DiskRatio int64
	MemRatio  int64
	CPURatio  int64

	DiskFreeSpaceRatio float64
	SegmentMirrorRatio int64
}

type LocalSSD struct {
	Step int64
	Min  int64
	Max  int64
}
type NetworkSSDNonReplicated struct {
	Step int64
	Min  int64
	Max  int64
}

type Dedicated struct {
	LocalSSD LocalSSD
}
type Shared struct {
	ClusterMaxDataSize      int64
	MaxCPU                  int64
	LocalSSD                map[int64]LocalSSD
	NetworkSSDNonReplicated NetworkSSDNonReplicated
}
type HostLimits struct {
	SegmentMaxCPUCount  int64
	SegmentMinHostCount int64
	Dedicated           Dedicated
	Shared              Shared
}
