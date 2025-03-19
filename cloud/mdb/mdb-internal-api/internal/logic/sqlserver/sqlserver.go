package sqlserver

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/ssmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	omodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/library/go/slices"
)

//go:generate ../../../../scripts/mockgen.sh SQLServer

const defaultSQLCollation = "Cyrillic_General_CI_AS"

type SQLServer interface {
	Cluster(ctx context.Context, cid string) (ssmodels.Cluster, error)
	Clusters(ctx context.Context, folderExtID string, limit, offset int64) ([]ssmodels.Cluster, error)
	CreateCluster(ctx context.Context, args CreateClusterArgs) (operations.Operation, error)
	ModifyCluster(ctx context.Context, args ModifyClusterArgs) (operations.Operation, error)
	DeleteCluster(ctx context.Context, cid string) (operations.Operation, error)
	BackupCluster(ctx context.Context, cid string) (operations.Operation, error)
	RestoreCluster(ctx context.Context, args RestoreClusterArgs) (operations.Operation, error)
	StartCluster(ctx context.Context, cid string) (operations.Operation, error)
	StopCluster(ctx context.Context, cid string) (operations.Operation, error)
	StartFailover(ctx context.Context, cid string, hostName string) (operations.Operation, error)
	UpdateHosts(ctx context.Context, cid string, specs []ssmodels.UpdateHostSpec) (operations.Operation, error)
	User(ctx context.Context, cid, name string) (ssmodels.User, error)
	Users(ctx context.Context, cid string, limit, offset int64) ([]ssmodels.User, error)
	UpdateUser(ctx context.Context, cid string, spec UserArgs) (operations.Operation, error)
	CreateUser(ctx context.Context, cid string, spec ssmodels.UserSpec) (operations.Operation, error)
	DeleteUser(ctx context.Context, cid string, name string) (operations.Operation, error)

	GrantPermission(ctx context.Context, cid, username string, permission ssmodels.Permission) (operations.Operation, error)
	RevokePermission(ctx context.Context, cid, username string, permission ssmodels.Permission) (operations.Operation, error)

	Database(ctx context.Context, cid, name string) (ssmodels.Database, error)
	Databases(ctx context.Context, cid string, limit, offset int64) ([]ssmodels.Database, error)
	CreateDatabase(ctx context.Context, cid string, spec ssmodels.DatabaseSpec) (operations.Operation, error)
	DeleteDatabase(ctx context.Context, cid string, name string) (operations.Operation, error)
	RestoreDatabase(ctx context.Context, cid string, spec ssmodels.RestoreDatabaseSpec) (operations.Operation, error)
	ImportDatabaseBackup(ctx context.Context, cid string, spec ssmodels.ImportDatabaseBackupSpec) (operations.Operation, error)
	ExportDatabaseBackup(ctx context.Context, cid string, spec ssmodels.ExportDatabaseBackupSpec) (operations.Operation, error)

	ListHosts(ctx context.Context, cid string) ([]hosts.HostExtended, error)
	ListBackups(ctx context.Context, cid string, pageToken bmodels.BackupsPageToken, pageSize int64) ([]bmodels.Backup, bmodels.BackupsPageToken, error)

	GetHostGroupType(ctx context.Context, hostGroupIds []string) (map[string]compute.HostGroupHostType, error)

	BackupByGlobalID(ctx context.Context, globalBackupID string) (bmodels.Backup, error)
	ListBackupsInFolder(ctx context.Context, folderExtID string, pageToken bmodels.BackupsPageToken, pageSize int64) ([]bmodels.Backup, bmodels.BackupsPageToken, error)
	EstimateCreateCluster(ctx context.Context, args CreateClusterArgs) (console.BillingEstimate, error)
	RestoreHints(ctx context.Context, globalBackupID string) (ssmodels.RestoreHints, error)
}

// NewClusterArgs is a common Cluster args used in Create/Restore operations
type NewClusterArgs struct {
	FolderExtID        string
	Name               string
	Description        string
	Labels             map[string]string
	Environment        environment.SaltEnv
	NetworkID          string
	SecurityGroupIDs   []string
	HostSpecs          []ssmodels.HostSpec
	ClusterConfigSpec  ssmodels.ClusterConfigSpec
	DeletionProtection bool
	HostGroupIDs       []string
	ServiceAccountID   string
}

func (args *NewClusterArgs) ValidateVersion() error {
	if args.ClusterConfigSpec.Version.String == "" {
		return semerr.InvalidInput("version must be specified")
	}
	isCorrect := false
	for _, version := range ssmodels.Versions {
		if version.ID == args.ClusterConfigSpec.Version.String {
			isCorrect = true
			break
		}
	}
	if !isCorrect {
		return semerr.InvalidInputf("unknown version: %s", args.ClusterConfigSpec.Version.String)
	}
	return nil
}

func (args *NewClusterArgs) Validate(rs resources.Preset, session sessions.Session, hostGroupHostType map[string]compute.HostGroupHostType) error {
	if err := models.ClusterNameValidator.ValidateString(args.Name); err != nil {
		return err
	}
	if err := models.ClusterDescriptionValidator.ValidateString(args.Description); err != nil {
		return err
	}
	if len(args.HostSpecs) == 0 {
		return semerr.InvalidInput("hosts must be specified")
	}
	if len(args.HostSpecs) != 1 && len(args.HostSpecs) != 3 &&
		ssmodels.VersionEdition(args.ClusterConfigSpec.Version.String) == ssmodels.EditionEnterprise &&
		!session.FeatureFlags.Has(ssmodels.SQLServerTwoNodeCluster) {
		return semerr.InvalidInput("only 1-node and 3-node clusters are supported now")
	}
	if len(args.HostSpecs) >= 2 && session.FeatureFlags.Has(ssmodels.SQLServerTwoNodeCluster) && ssmodels.VersionEdition(args.ClusterConfigSpec.Version.String) == ssmodels.EditionStandard {
		if len(args.HostSpecs) > 2 {
			return semerr.InvalidInput("only 2-node clusters are supported for Standard edition")
		}
		if args.ClusterConfigSpec.SecondaryConnections.GetOrDefault() == 1 {
			return semerr.InvalidInput("readable secondaries are not supported for Standard edition")
		}
	}
	if err := args.ClusterConfigSpec.Validate(rs, true); err != nil {
		return err
	}
	if err := args.ValidateVersion(); err != nil {
		return err
	}
	if ssmodels.VersionEdition(args.ClusterConfigSpec.Version.String) == ssmodels.EditionStandard {
		if len(args.HostSpecs) != 1 && !session.FeatureFlags.Has(ssmodels.SQLServerTwoNodeCluster) {
			return semerr.InvalidInput("only 1-node clusters are supported for Standard edition now")
		}
		if rs.CPUGuarantee > ssmodels.StandardEditionMaxCores || rs.MemoryGuarantee > ssmodels.StandaddEditionMaxMemory {
			return semerr.InvalidInputf("maximum %d cores and %d Gb of memory supported for Standard edition",
				ssmodels.StandardEditionMaxCores, ssmodels.StandaddEditionMaxMemory/(1024*1024*1024))
		}
	}
	if len(hostGroupHostType) > 0 {
		if err := args.validateHostGroupResources(rs, hostGroupHostType); err != nil {
			return err
		}
	}
	return nil
}

func (args *NewClusterArgs) validateHostGroupResources(rs resources.Preset, hostGroupHostType map[string]compute.HostGroupHostType) error {
	if len(hostGroupHostType) == 0 {
		return nil
	}
	minCores, minMemory, minDisks, diskSize, diskSizeHostGroup := compute.GetMinHostGroupResources(hostGroupHostType, "")
	if rs.MemoryLimit > minMemory {
		return semerr.InvalidInputf("cannot use resource preset %q on dedicated host, max memory is %d", args.ClusterConfigSpec.Resources.ResourcePresetExtID.String, minMemory)
	}
	if rs.CPULimit > float64(minCores) {
		return semerr.InvalidInputf("cannot use resource preset %q on dedicated host, max cpu limit is %d", args.ClusterConfigSpec.Resources.ResourcePresetExtID.String, minCores)
	}
	err := args.ClusterConfigSpec.Resources.Validate(true)
	if err != nil {
		return err
	}

	if args.ClusterConfigSpec.Resources.DiskTypeExtID.String == resources.LocalSSD {
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
		if args.ClusterConfigSpec.Resources.DiskTypeExtID.String == resources.LocalSSD {
			if maxDiskSize < args.ClusterConfigSpec.Resources.DiskSize.Int64 {
				return semerr.InvalidInputf("cannot use disk size %d on host group, max disk size is %d", args.ClusterConfigSpec.Resources.DiskSize.Int64, maxDiskSize)
			}
			if args.ClusterConfigSpec.Resources.DiskSize.Int64%diskSize != 0 {
				return semerr.InvalidInputf("on host group disk size should by multiple of %d", diskSize)
			}
		}
	}
	return nil
}

type CreateClusterArgs struct {
	NewClusterArgs
	UserSpecs     []ssmodels.UserSpec
	DatabaseSpecs []ssmodels.DatabaseSpec
	SQLCollation  string
}

func (args *CreateClusterArgs) ValidateCreate() error {
	if err := args.NewClusterArgs.ValidateVersion(); err != nil {
		return err
	}
	if args.SQLCollation == "" {
		args.SQLCollation = defaultSQLCollation
	}
	_, ok := AllowedCollations[args.ClusterConfigSpec.Version.String][args.SQLCollation]
	if !ok {
		return semerr.InvalidInputf("SQLCollation %s is not valid", args.SQLCollation)
	}
	return nil
}

type RestoreClusterArgs struct {
	NewClusterArgs
	BackupID string
	Time     time.Time
}

func (args *RestoreClusterArgs) ValidateRestore(backup bmodels.Backup, sourceResource models.ClusterResources) error {
	if args.Time.IsZero() {
		return semerr.InvalidInput("restore timestamp must be specified")
	}
	if err := backup.ValidateRestoreTime(args.Time); err != nil {
		return err
	}
	if args.ClusterConfigSpec.Resources.DiskSize.Int64 < sourceResource.DiskSize {
		return semerr.FailedPreconditionf("insufficient diskSize, increase it to '%d'", sourceResource.DiskSize)
	}
	return nil
}

type ModifyClusterArgs struct {
	ClusterID          string
	SecurityGroupsIDs  optional.Strings
	Name               optional.String
	Description        optional.String
	Labels             omodels.Labels
	ClusterConfigSpec  ssmodels.ClusterConfigSpec
	DeletionProtection optional.Bool
	ServiceAccountID   optional.String
}

func (args *ModifyClusterArgs) Validate(rs resources.Preset) error {
	if args.Name.Valid {
		if err := models.ClusterNameValidator.ValidateString(args.Name.String); err != nil {
			return err
		}
	}
	if args.Description.Valid {
		if err := models.ClusterDescriptionValidator.ValidateString(args.Description.String); err != nil {
			return err
		}
	}
	if err := args.ClusterConfigSpec.Validate(rs, false); err != nil {
		return err
	}
	return nil
}

type UserArgs struct {
	Name      string
	ClusterID string

	Password    optional.OptionalPassword
	Permissions ssmodels.OptionalPermissions
	ServerRoles ssmodels.OptionalServerRoles
}

func UserArgsFromUserSpec(us ssmodels.UserSpec) UserArgs {
	return UserArgs{
		Name:        us.Name,
		Password:    optional.NewOptionalPassword(us.Password),
		Permissions: ssmodels.NewOptionalPermissions(us.Permissions),
		ServerRoles: ssmodels.NewOptionalServerRoles(us.ServerRoles),
	}
}

func (ua UserArgs) Validate(validDBNames []string) error {
	// validate username
	if err := ssmodels.UserNameValidator.ValidateString(ua.Name); err != nil {
		return err
	}

	// validate password
	if ua.Password.Valid {
		if err := ssmodels.UserPasswordValidator.ValidateString(ua.Password.Password.Unmask()); err != nil {
			return err
		}
	}

	// validate databases
	if ua.Permissions.Valid {
		dbnames := make(map[string]struct{})
		for _, p := range ua.Permissions.Permissions {
			if !slices.ContainsString(validDBNames, p.DatabaseName) {
				return semerr.InvalidInputf("permission refers nonexistent database: %s", p.DatabaseName)
			}

			if err := p.Validate(); err != nil {
				return err
			}

			if _, ok := dbnames[p.DatabaseName]; ok {
				return semerr.InvalidInputf("duplicate permission for database %s", p.DatabaseName)
			}

			dbnames[p.DatabaseName] = struct{}{}
		}
	}

	return nil
}
