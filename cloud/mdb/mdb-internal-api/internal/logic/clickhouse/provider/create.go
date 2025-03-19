package provider

import (
	"context"
	"time"

	cheventspub "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events/mdb/clickhouse"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/provider/internal/chpillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/compute"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	taskslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type shardSpec struct {
	ShardName   string
	ShardWeight int64
	ShardHosts  []chmodels.HostSpec
}

type createClusterImplArgs struct {
	FolderExtID           string
	Name                  string
	Description           string
	Labels                map[string]string
	CloudType             environment.CloudType
	RegionID              optional.String
	ClusterSpec           chmodels.CombinedClusterSpec
	BackupWindowStart     bmodels.OptionalBackupWindowStart
	Environment           environment.SaltEnv
	NetworkID             string
	SecurityGroupIDs      []string
	UserSpecs             []chmodels.UserSpec
	DatabaseSpecs         []chmodels.DatabaseSpec
	Shards                []shardSpec
	ClickHouseHostGroups  clusterslogic.ResolvedHostGroups
	ClusterHosts          chmodels.ClusterHosts
	DeletionProtection    bool
	MaintenanceWindow     clusters.MaintenanceWindow
	ServiceAccountID      string
	ClickHouseClusterName string
}

func (ch *ClickHouse) prepareMDBClusterCreate(
	ctx context.Context,
	session sessions.Session,
	preparer prepareModifier,
	args clickhouse.CreateMDBClusterArgs,
) (createClusterImplArgs, error) {
	if err := args.ValidateAndSane(); err != nil {
		return createClusterImplArgs{}, err
	}

	if err := ch.validateGeoBaseURI(ctx, args.ClusterSpec.Config.GeobaseURI); err != nil {
		return createClusterImplArgs{}, nil
	}

	v, err := ch.validateClickHouseVersion(session, args.ClusterSpec.Version)
	args.ClusterSpec.Version = v
	if err != nil {
		return createClusterImplArgs{}, err
	}

	hostSpecs, err := chmodels.SplitHostsByType(args.HostSpecs)
	if err != nil {
		return createClusterImplArgs{}, err
	}

	clickHouseResources, err := ch.getDefaultClickHouseResources(preparer)
	if err != nil {
		return createClusterImplArgs{}, err
	}
	clickHouseResources.Merge(args.ClusterSpec.ClickHouseResources)
	if err := clickHouseResources.Validate(); err != nil {
		return createClusterImplArgs{}, err
	}

	resolvedGroups, _, err := preparer.ValidateResources(ctx, session, clusters.TypeClickHouse, clusterslogic.HostGroup{
		Role:                   hosts.RoleClickHouse,
		NewResourcePresetExtID: optional.NewString(clickHouseResources.ResourcePresetExtID),
		DiskTypeExtID:          clickHouseResources.DiskTypeExtID,
		NewDiskSize:            optional.NewInt64(clickHouseResources.DiskSize),
		HostsToAdd:             chmodels.ToZoneHosts(hostSpecs.ClickHouseNodes),
		ShardName:              optional.NewString(args.ShardName),
	})
	if err != nil {
		return createClusterImplArgs{}, err
	}

	if args.ServiceAccountID != "" {
		if resolvedGroups.Single().TargetResourcePreset().VType != environment.VTypeCompute {
			return createClusterImplArgs{}, semerr.FailedPrecondition("service accounts are not supported in porto clusters")
		}

		if err := session.ValidateServiceAccount(ctx, args.ServiceAccountID); err != nil {
			return createClusterImplArgs{}, err
		}
	}

	if args.ClusterSpec.EnableCloudStorage.Bool {
		if err := clickhouse.ValidateCloudStorageSupported(args.ClusterSpec.Version); err != nil {
			return createClusterImplArgs{}, err
		}
	}

	if args.ClusterSpec.EmbeddedKeeper {
		if err := clickhouse.ValidateClickHouseKeeperSupported(args.ClusterSpec.Version); err != nil {
			return createClusterImplArgs{}, err
		}
	}

	return createClusterImplArgs{
		FolderExtID:          args.FolderExtID,
		Name:                 args.Name,
		Description:          args.Description,
		Labels:               args.Labels,
		ClusterSpec:          args.ClusterSpec.Combine(),
		BackupWindowStart:    args.ClusterSpec.BackupWindowStart,
		Environment:          args.Environment,
		NetworkID:            args.NetworkID,
		SecurityGroupIDs:     args.SecurityGroupIDs,
		UserSpecs:            args.UserSpecs,
		DatabaseSpecs:        args.DatabaseSpecs,
		ClickHouseHostGroups: resolvedGroups,
		ClusterHosts:         hostSpecs,
		DeletionProtection:   args.DeletionProtection,
		MaintenanceWindow:    args.MaintenanceWindow,
		ServiceAccountID:     args.ServiceAccountID,
		Shards: []shardSpec{{
			ShardName:   args.ShardName,
			ShardWeight: chmodels.DefaultShardWeight,
			ShardHosts:  hostSpecs.ClickHouseNodes,
		}},
	}, nil
}

func (ch *ClickHouse) CreateMDBCluster(ctx context.Context, args clickhouse.CreateMDBClusterArgs) (operations.Operation, error) {
	return ch.operator.Create(ctx, args.FolderExtID, func(ctx context.Context, session sessions.Session, creator clusterslogic.Creator) (clusters.Cluster, operations.Operation, error) {
		implArgs, err := ch.prepareMDBClusterCreate(ctx, session, creator, args)
		if err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}

		return ch.createClusterImpl(ctx, session, creator, implArgs)
	})
}

func (ch *ClickHouse) prepareDataCloudClusterCreate(
	ctx context.Context,
	session sessions.Session,
	creator clusterslogic.Creator,
	args clickhouse.CreateDataCloudClusterArgs,
) (createClusterImplArgs, error) {
	if err := args.ValidateAndSane(); err != nil {
		return createClusterImplArgs{}, err
	}

	v, err := ch.validateClickHouseVersion(session, args.ClusterSpec.Version)
	args.ClusterSpec.Version = v
	if err != nil {
		return createClusterImplArgs{}, err
	}

	chZones, err := selectHostZones(ctx, session, creator, int(args.ReplicaCount), nil, optional.NewString(args.RegionID))
	if err != nil {
		return createClusterImplArgs{}, err
	}

	region, err := creator.RegionByName(ctx, args.RegionID)
	if err != nil {
		return createClusterImplArgs{}, err
	}
	backupTime, err := time.ParseInLocation(time.ANSIC, chmodels.DefaultBackupTime, region.TimeZone)
	if err != nil {
		return createClusterImplArgs{}, xerrors.Errorf("parse default backup time: %+v", err)
	}
	backupTime = backupTime.UTC()
	backupWindow := bmodels.BackupWindowStart{
		Hours:   backupTime.Hour(),
		Minutes: backupTime.Minute(),
		Seconds: backupTime.Second(),
		Nanos:   backupTime.Nanosecond(),
	}

	diskType, err := creator.DiskTypeExtIDByResourcePreset(ctx, clusters.TypeClickHouse, hosts.RoleClickHouse,
		args.ClusterSpec.ClickHouseResources.ResourcePresetExtID.String, chZones, session.FeatureFlags.List())
	if err != nil {
		return createClusterImplArgs{}, err
	}
	args.ClusterSpec.ClickHouseResources.DiskTypeExtID = optional.NewString(diskType)

	shards := make([]shardSpec, 0, args.ShardCount.Int64)
	hostSpecs := make([]chmodels.HostSpec, 0, len(chZones)*int(args.ShardCount.Int64))
	hostGroups := make([]clusterslogic.HostGroup, 0, args.ShardCount.Int64)
	for i := 1; i <= int(args.ShardCount.Int64); i++ {
		shardHostSpecs := make([]chmodels.HostSpec, 0, len(chZones))
		name := chmodels.GenerateShardName(i)
		for _, zone := range chZones {
			shardHostSpecs = append(shardHostSpecs, chmodels.HostSpec{
				ZoneID:         zone,
				AssignPublicIP: true,
				HostRole:       hosts.RoleClickHouse,
				ShardName:      name,
			})
		}

		hostSpecs = append(hostSpecs, shardHostSpecs...)
		shards = append(shards, shardSpec{
			ShardName:   name,
			ShardWeight: chmodels.DefaultShardWeight,
			ShardHosts:  shardHostSpecs,
		})

		hostGroups = append(hostGroups, clusterslogic.HostGroup{
			Role:                   hosts.RoleClickHouse,
			NewResourcePresetExtID: args.ClusterSpec.ClickHouseResources.ResourcePresetExtID,
			DiskTypeExtID:          args.ClusterSpec.ClickHouseResources.DiskTypeExtID.Must(),
			NewDiskSize:            args.ClusterSpec.ClickHouseResources.DiskSize,
			HostsToAdd:             chmodels.ToZoneHosts(shardHostSpecs),
			ShardName:              optional.NewString(name),
		})
	}

	resolvedGroups, _, err := creator.ValidateResources(ctx, session, clusters.TypeClickHouse, hostGroups...)
	if err != nil {
		return createClusterImplArgs{}, err
	}

	if err := args.ClusterSpec.Access.ValidateAndSane(); err != nil {
		return createClusterImplArgs{}, err
	}

	clusterSpec := args.ClusterSpec.Combine()
	clusterSpec.EmbeddedKeeper = true
	clusterSpec.SQLUserManagement = optional.NewBool(true)
	clusterSpec.SQLDatabaseManagement = optional.NewBool(true)
	clusterSpec.MySQLProtocol = optional.NewBool(false)
	clusterSpec.PostgreSQLProtocol = optional.NewBool(false)
	clusterSpec.EnableCloudStorage = optional.NewBool(true)

	cloudStorageDataCacheSupported, err := chmodels.VersionGreaterOrEqual(args.ClusterSpec.Version, 22, 5)
	if err != nil {
		return createClusterImplArgs{}, err
	}

	if cloudStorageDataCacheSupported {
		clusterSpec.CloudStorageConfig.DataCacheEnabled = optional.NewBool(true)
		clusterSpec.CloudStorageConfig.DataCacheMaxSize = args.ClusterSpec.ClickHouseResources.DiskSize
		clusterSpec.CloudStorageConfig.DataCacheMaxSize.Int64 /= 2
	}

	adminPass, err := ch.cryptoProvider.GenerateRandomString(chmodels.PasswordLen, []rune(crypto.PasswordValidRunes))
	if err != nil {
		return createClusterImplArgs{}, err
	}
	clusterSpec.AdminPassword = optional.NewOptionalPassword(adminPass)

	if !args.NetworkID.Valid {
		network, err := compute.GetOrCreateNetwork(ctx, ch.compute, args.ProjectID, args.RegionID)
		if err != nil {
			return createClusterImplArgs{}, err
		}

		args.NetworkID.Set(network.ID)
	}

	return createClusterImplArgs{
		FolderExtID: args.ProjectID,
		Name:        args.Name,
		Description: args.Description,
		CloudType:   args.CloudType,
		RegionID:    optional.NewString(args.RegionID),
		ClusterSpec: clusterSpec,
		BackupWindowStart: bmodels.OptionalBackupWindowStart{
			Value: backupWindow,
			Valid: true,
		},
		Environment:           ch.cfg.SaltEnvs.Production,
		Shards:                shards,
		ClickHouseHostGroups:  resolvedGroups,
		ClusterHosts:          chmodels.ClusterHosts{ClickHouseNodes: hostSpecs},
		ClickHouseClusterName: "default",
		NetworkID:             args.NetworkID.String,
	}, nil
}

func (ch *ClickHouse) prepareDataCloudClusterCreateEstimation(
	ctx context.Context,
	session sessions.Session,
	creator clusterslogic.Creator,
	args clickhouse.CreateDataCloudClusterArgs,
) (createClusterImplArgs, error) {
	if err := args.ValidateAndSane(); err != nil {
		return createClusterImplArgs{}, err
	}

	chZones, err := selectHostZones(ctx, session, creator, int(args.ReplicaCount), nil, optional.NewString(args.RegionID))
	if err != nil {
		return createClusterImplArgs{}, err
	}

	diskType, err := creator.DiskTypeExtIDByResourcePreset(ctx, clusters.TypeClickHouse, hosts.RoleClickHouse,
		args.ClusterSpec.ClickHouseResources.ResourcePresetExtID.String, chZones, session.FeatureFlags.List())
	if err != nil {
		return createClusterImplArgs{}, err
	}
	args.ClusterSpec.ClickHouseResources.DiskTypeExtID = optional.NewString(diskType)

	hostSpecs := make([]chmodels.HostSpec, 0, len(chZones)*int(args.ShardCount.Int64))
	for i := 1; i <= int(args.ShardCount.Int64); i++ {
		shardHostSpecs := make([]chmodels.HostSpec, 0, len(chZones))
		for _, zone := range chZones {
			shardHostSpecs = append(shardHostSpecs, chmodels.HostSpec{
				ZoneID:         zone,
				AssignPublicIP: true,
				HostRole:       hosts.RoleClickHouse,
			})
		}

		hostSpecs = append(hostSpecs, shardHostSpecs...)
	}

	return createClusterImplArgs{
		ClusterSpec:  args.ClusterSpec.Combine(),
		ClusterHosts: chmodels.ClusterHosts{ClickHouseNodes: hostSpecs},
	}, nil
}

func (ch *ClickHouse) CreateDataCloudCluster(ctx context.Context, args clickhouse.CreateDataCloudClusterArgs) (operations.Operation, error) {
	return ch.operator.Create(
		ctx, args.ProjectID,
		func(ctx context.Context, session sessions.Session, creator clusterslogic.Creator) (clusters.Cluster, operations.Operation, error) {

			implArgs, err := ch.prepareDataCloudClusterCreate(ctx, session, creator, args)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			return ch.createClusterImpl(ctx, session, creator, implArgs)
		})
}

func (ch *ClickHouse) createClusterImpl(
	ctx context.Context,
	session sessions.Session,
	creator clusterslogic.Creator,
	args createClusterImplArgs,
) (clusters.Cluster, operations.Operation, error) {
	network, subnets, err := ch.compute.NetworkAndSubnets(ctx, args.NetworkID)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	if len(args.SecurityGroupIDs) > 0 {
		if err := ch.compute.ValidateSecurityGroups(ctx, args.SecurityGroupIDs, network.ID); err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}
	}

	chCluster, err := ch.createCluster(ctx, session, creator, args, network)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	backupSchedule := bmodels.GetBackupSchedule(ch.cfg.ClickHouse.Backup.BackupSchedule, args.BackupWindowStart)
	if err := ch.backups.AddBackupSchedule(ctx, chCluster.ClusterID, backupSchedule, chCluster.Revision); err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("couldn't create chCluster during adding backup schedule: %w ", err)
	}

	pillar, err := ch.createPillar(session, chCluster, args, backupSchedule)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	s3Buckets := map[string]string{"backup": ch.cfg.S3BucketName(chCluster.ClusterID)}
	if args.ClusterSpec.EnableCloudStorage.Bool {
		s3Buckets["cloud_storage"] = ch.cfg.ClickHouse.CloudStorageBucketName(chCluster.ClusterID)
	}

	resolvedGroups := args.ClickHouseHostGroups
	if !args.ClusterSpec.EmbeddedKeeper && args.ClusterHosts.NeedZookeeper() {
		zkResources, zkHostSpecs, err := ch.buildZookeeperResources(ctx, session, creator, args.ClickHouseHostGroups.MustGroupsByHostRole(hosts.RoleClickHouse),
			args.ClusterHosts, []hosts.HostExtended{}, args.ClusterSpec.ZookeeperResources, args.RegionID, subnets)
		if err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}

		args.ClusterHosts.ZooKeeperNodes = zkHostSpecs

		resolvedZkGroups, _, err := creator.ValidateResources(ctx, session, clusters.TypeClickHouse, clusterslogic.HostGroup{
			Role:                   hosts.RoleZooKeeper,
			NewResourcePresetExtID: optional.NewString(zkResources.ResourcePresetExtID),
			DiskTypeExtID:          zkResources.DiskTypeExtID,
			NewDiskSize:            optional.NewInt64(zkResources.DiskSize),
			HostsToAdd:             chmodels.ToZoneHosts(args.ClusterHosts.ZooKeeperNodes),
		})
		if err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}

		resolvedGroups = clusterslogic.NewResolvedHostGroups(append(args.ClickHouseHostGroups.MustGroupsByHostRole(hosts.RoleClickHouse), resolvedZkGroups.Single()))

		if err := ch.validateZookeeperCores(ctx, session, creator, resolvedGroups, ""); err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}

		if err := ch.createZKSubCluster(ctx, session, creator, chCluster, args.ClusterHosts.ZooKeeperNodes, resolvedZkGroups.Single(), subnets); err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}
	}

	chSubCluster, err := ch.createCHSubCluster(ctx, session, creator, chCluster, pillar, resolvedGroups.MustMapByShardName(hosts.RoleClickHouse), subnets, args.Shards)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	createClusterTaskArgs := map[string]interface{}{
		"update-geobase":     args.ClusterSpec.Config.GeobaseURI.Valid,
		"service_account_id": pillar.Data.ServiceAccountID,
		"s3_buckets":         s3Buckets,
	}
	if ch.cfg.ClickHouse.Backup.BackupSchedule.UseBackupService {
		var initialBackupInfo []map[string]string
		for shardID := range chSubCluster.Pillar.Data.ClickHouse.Shards {
			backupID, err := ch.backups.GenerateNewBackupID()
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}
			initialBackupInfo = append(initialBackupInfo, map[string]string{
				"backup_id": backupID,
				"subcid":    chSubCluster.SubClusterID,
				"shard_id":  shardID,
			})
		}
		createClusterTaskArgs["backup_service_initial_info"] = initialBackupInfo
	}

	op, err := ch.tasks.CreateCluster(
		ctx,
		session,
		chCluster.ClusterID,
		chCluster.Revision,
		chmodels.TaskTypeClusterCreate,
		chmodels.OperationTypeClusterCreate,
		chmodels.MetadataCreateCluster{},
		optional.String{}, // We pass additional buckets via options
		args.SecurityGroupIDs,
		clickHouseService,
		searchAttributesExtractor(chSubCluster),
		func(options *taskslogic.CreateClusterOptions) {
			options.TaskArgs = createClusterTaskArgs
			options.Timeout = optional.NewDuration(getTimeout(optional.NewDuration(time.Hour*3), options.TaskArgs, resolvedGroups))
		},
	)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	event := &cheventspub.CreateCluster{
		Authentication:  ch.events.NewAuthentication(session.Subject),
		Authorization:   ch.events.NewAuthorization(session.Subject),
		RequestMetadata: ch.events.NewRequestMetadata(ctx),
		EventStatus:     cheventspub.CreateCluster_STARTED,
		Details: &cheventspub.CreateCluster_EventDetails{
			ClusterId: chCluster.ClusterID,
		},
	}

	em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}
	event.EventMetadata = em

	if err = ch.events.Store(ctx, event, op); err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("saving event for op %+v: %w", op, err)
	}

	return chCluster.Cluster, op, nil
}

func (ch *ClickHouse) createCluster(ctx context.Context, session sessions.Session, creator clusterslogic.Creator, args createClusterImplArgs, network networkProvider.Network) (cluster, error) {
	chCluster, privateKey, err := creator.CreateCluster(ctx, models.CreateClusterArgs{
		Name:               args.Name,
		ClusterType:        clusters.TypeClickHouse,
		Environment:        args.Environment,
		NetworkID:          network.ID,
		FolderID:           session.FolderCoords.FolderID,
		Description:        args.Description,
		Labels:             args.Labels,
		DeletionProtection: args.DeletionProtection,
		MaintenanceWindow:  args.MaintenanceWindow,
	})
	if err != nil {
		return cluster{}, err
	}

	pillar := chpillars.NewClusterCH()
	pillar.SetEncryption(args.ClusterSpec.Encryption)
	pillar.Data.S3Bucket = ch.cfg.S3BucketName(chCluster.ClusterID)
	encryptedPrivateKey, err := ch.cryptoProvider.Encrypt(privateKey)
	if err != nil {
		return cluster{}, err
	}
	pillar.Data.ClusterPrivateKey = encryptedPrivateKey
	pillar.Data.CloudType = args.CloudType
	pillar.Data.RegionID = args.RegionID.String
	if ch.cfg.E2E.IsClusterE2E(args.Name, args.FolderExtID) {
		pillar.Data.MDBMetrics = pillars.NewDisabledMDBMetrics()
		pillar.Data.UseYASMAgent = new(bool)
		pillar.Data.SuppressExternalYASMAgent = true
		pillar.Data.Billing = pillars.NewDisabledBilling()
		pillar.Data.ShipLogs = new(bool)
		pillar.Data.MDBHealth = pillars.NewMDBHealthWithDisabledAggregate()
	}

	if err := creator.AddClusterPillar(ctx, chCluster.ClusterID, chCluster.Revision, pillar); err != nil {
		return cluster{}, err
	}

	return cluster{chCluster, pillar}, nil
}

type createPillarOptions struct {
	AdminPassword *chpillars.AdminPassword
}

func (ch *ClickHouse) createPillar(session sessions.Session, cluster cluster, args createClusterImplArgs, backupSchedule bmodels.BackupSchedule, opts ...createPillarOptions) (*chpillars.SubClusterCH, error) {
	pillar := chpillars.NewSubClusterCH()
	password, err := crypto.GenerateEncryptedPassword(ch.cryptoProvider, chmodels.PasswordLen, nil)
	if err != nil {
		return nil, err
	}
	pillar.Data.ClickHouse.InterserverCredentials = chpillars.InterserverCredentials{
		User:     "interserver",
		Password: password,
	}

	if args.ClusterSpec.SQLDatabaseManagement.Bool && !args.ClusterSpec.SQLUserManagement.Bool {
		return nil, semerr.InvalidInput("SQL database management is not supported without SQL user management")
	}

	if args.ClusterSpec.SQLUserManagement.Bool && !args.ClusterSpec.AdminPassword.Valid {
		if len(opts) == 0 || opts[0].AdminPassword == nil {
			return nil, semerr.InvalidInputf("admin password must be specified in order to enable SQL user management")
		}
	}

	if args.ClusterSpec.SQLDatabaseManagement.Bool && len(args.DatabaseSpecs) > 0 {
		return nil, semerr.InvalidInput("databases to create cannot be specified when SQL database management is enabled")
	}
	for _, database := range args.DatabaseSpecs {
		if err := pillar.AddDatabase(database); err != nil {
			return nil, err
		}
	}

	if args.ClusterSpec.SQLUserManagement.Bool && len(args.UserSpecs) > 0 {
		return nil, semerr.InvalidInput("users to create cannot be specified when SQL user management is enabled")
	}
	for _, user := range args.UserSpecs {
		pass, hash, err := ch.encryptUserPassword(user.Password)
		if err != nil {
			return nil, err
		}

		if err := pillar.AddUser(user, pass, hash); err != nil {
			return nil, err
		}
	}

	pillar.Data.ClickHouse.Version = args.ClusterSpec.Version

	zkPass, err := crypto.GenerateEncryptedPassword(ch.cryptoProvider, chmodels.PasswordLen, nil)
	if err != nil {
		return nil, err
	}
	pillar.Data.ClickHouse.ZKUsers = map[string]chpillars.ZooKeeperCredentials{
		chmodels.ZKACLUserClickhouse: {Password: zkPass},
	}

	mdbBackupAdminPass, err := crypto.GenerateRawPassword(ch.cryptoProvider, chmodels.PasswordLen, nil)
	if err != nil {
		return nil, err
	}
	mdbBackupAdminPassEnc, mdbBackupHashPass, err := ch.encryptUserPassword(mdbBackupAdminPass)
	if err != nil {
		return nil, err
	}
	pillar.Data.ClickHouse.SystemUsers = map[string]chpillars.SystemUserCredentials{
		chmodels.SystemUserBackupAdmin: {Password: mdbBackupAdminPassEnc, Hash: mdbBackupHashPass},
	}

	pillar.Data.ClickHouse.Config.MergeTree.EnableMixedGranularityParts = pillars.MapOptionalBoolToPtrBool(optional.NewBool(true))
	if err := pillar.SetClickHouseConfig(ch.cryptoProvider, args.ClusterSpec.Config); err != nil {
		return nil, err
	}

	pillar.Data.Backup = backupSchedule

	pillar.SetAccess(args.ClusterSpec.Access)
	pillar.EnableCloudStorage(args.ClusterSpec.EnableCloudStorage.Bool, cluster.ClusterID, ch.cfg.ClickHouse)
	pillar.SetServiceAccountID(args.ServiceAccountID)
	pillar.SetSQLUserManagement(args.ClusterSpec.SQLUserManagement)
	pillar.SetSQLDatabaseManagement(args.ClusterSpec.SQLDatabaseManagement)
	pillar.SetClickHouseClusterName(args.ClickHouseClusterName)

	if args.ClusterSpec.AdminPassword.Valid {
		pass, hash, err := ch.encryptUserPassword(args.ClusterSpec.AdminPassword.Password)
		if err != nil {
			return nil, err
		}
		pillar.Data.ClickHouse.AdminPassword = &chpillars.AdminPassword{
			Hash:     hash,
			Password: pass,
		}
	}

	userManagementV2Supported, err := chmodels.VersionGreaterOrEqual(args.ClusterSpec.Version, 20, 6)
	if err != nil {
		return nil, err
	}
	pillar.Data.ClickHouse.UserManagementV2 = &userManagementV2Supported

	testingVersions := session.FeatureFlags.Has(chmodels.ClickHouseTestingVersionsFeatureFlag)
	pillar.Data.TestingRepos = &testingVersions

	pillar.SetEmbeddedKeeper(args.ClusterSpec.EmbeddedKeeper)
	pillar.SetMySQLProtocol(args.ClusterSpec.MySQLProtocol)
	pillar.SetPostgreSQLProtocol(args.ClusterSpec.PostgreSQLProtocol)

	pillar.SetAccess(args.ClusterSpec.Access)

	if args.ClusterSpec.CloudStorageConfig.Valid() {
		if pillar.Data.CloudStorage.Settings == nil {
			pillar.Data.CloudStorage.Settings = &chpillars.CloudStorageSettings{}
		}

		pillar.Data.CloudStorage.Settings.MoveFactor = pillars.MapOptionalFloat64ToPtrFloat64(args.ClusterSpec.CloudStorageConfig.MoveFactor)
		pillar.Data.CloudStorage.Settings.DataCacheEnabled = pillars.MapOptionalBoolToPtrBool(args.ClusterSpec.CloudStorageConfig.DataCacheEnabled)
		pillar.Data.CloudStorage.Settings.DataCacheMaxSize = pillars.MapOptionalInt64ToPtrInt64(args.ClusterSpec.CloudStorageConfig.DataCacheMaxSize)
	}

	return pillar, nil
}

func (ch *ClickHouse) createCHSubCluster(ctx context.Context, session sessions.Session, creator clusterslogic.Creator, cluster cluster, pillar *chpillars.SubClusterCH,
	groupMap map[string]clusterslogic.ResolvedHostGroup, subnets []networkProvider.Subnet, shards []shardSpec) (subCluster, error) {
	sc, err := creator.CreateSubCluster(ctx, models.CreateSubClusterArgs{
		ClusterID: cluster.ClusterID,
		Name:      chmodels.CHSubClusterName,
		Roles:     []hosts.Role{hosts.RoleClickHouse},
		Revision:  cluster.Revision,
	})
	if err != nil {
		return subCluster{}, err
	}
	chSubCluster := subCluster{sc, pillar}

	var newHosts []hosts.Host
	for _, shard := range shards {
		s, createdHosts, err := ch.createShard(ctx, session, creator, cluster, chSubCluster, groupMap[shard.ShardName], subnets, clickhouse.CreateShardArgs{
			Name: shard.ShardName,
			ConfigSpec: clickhouse.ShardConfigSpec{
				Weight: optional.NewInt64(shard.ShardWeight),
			},
			HostSpecs: shard.ShardHosts,
		})
		if err != nil {
			return subCluster{}, err
		}

		if err := pillar.AddShard(s.ShardID, shard.ShardWeight); err != nil {
			return subCluster{}, err
		}
		newHosts = append(newHosts, createdHosts...)
	}

	if pillar.EmbeddedKeeper() {
		pillar.SetEmbeddedKeeperHosts(chooseNewKeeperHosts(newHosts, []hosts.Host{}))
	}

	err = creator.AddSubClusterPillar(ctx, cluster.ClusterID, sc.SubClusterID, cluster.Revision, pillar)
	if err != nil {
		return subCluster{}, err
	}

	return chSubCluster, err
}

func searchAttributesExtractor(ch subCluster) search.AttributesExtractor {
	var users []string

	for user := range ch.Pillar.Data.ClickHouse.Users {
		users = append(users, user)
	}

	return func(clusterslogic.Cluster) (map[string]interface{}, error) {
		return map[string]interface{}{
			"users":     users,
			"databases": ch.Pillar.Data.ClickHouse.DBs,
		}, nil
	}
}
