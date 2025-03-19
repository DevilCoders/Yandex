package provider

import (
	"context"

	intcompute "a.yandex-team.ru/cloud/mdb/internal/compute"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/compute"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/provider/internal/kfpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

const (
	passwordGenLen = 16
)

type createClusterImplArgs struct {
	Name               string
	CloudType          environment.CloudType
	RegionID           string
	FolderExtID        string
	ConfigSpec         kfmodels.ClusterConfigSpec
	UserSpecs          []kfmodels.UserSpec
	TopicSpecs         []kfmodels.TopicSpec
	Environment        environment.SaltEnv
	IsPrestable        bool
	NetworkID          string
	SubnetID           []string
	Description        string
	Labels             clusters.Labels
	HostGroupIDs       []string
	DeletionProtection bool
	SyncTopics         bool
	MaintenanceWindow  clusters.MaintenanceWindow
	SecurityGroupIDs   []string
	ZkZones            []string
	OneHostMode        bool
	ZkScramEnabled     bool
	AdminUserName      string
}

func (args createClusterImplArgs) Validate() error {
	if args.FolderExtID == "" {
		return semerr.InvalidInput("folder id must be specified") // TODO use resource model when go to DataCloud
	}
	if args.Name == "" {
		return semerr.InvalidInput("cluster name must be specified")
	}
	if args.ConfigSpec.BrokersCount < 1 {
		return semerr.InvalidInput("brokers count must be at least 1")
	}
	if len(args.ConfigSpec.ZoneID) == 0 {
		return semerr.InvalidInput("at least one zone should be specified")
	}

	if err := args.ConfigSpec.Validate(); err != nil {
		return err
	}

	if !args.IsPrestable && args.ConfigSpec.Version == "3.2" {
		return semerr.NotImplemented("version 3.2 can be created only in prestable environment")
	}

	for _, topic := range args.TopicSpecs {
		if err := topic.Validate(); err != nil {
			return err
		}
	}

	for _, user := range args.UserSpecs {
		if err := user.Validate(); err != nil {
			return err
		}
	}

	return models.ClusterNameValidator.ValidateString(args.Name)
}

func (kf *Kafka) CreateMDBCluster(ctx context.Context, args kafka.CreateMDBClusterArgs) (operations.Operation, error) {
	return kf.operator.Create(ctx, args.FolderExtID,
		func(ctx context.Context, session sessions.Session, creator clusterslogic.Creator) (clusters.Cluster, operations.Operation, error) {
			if !session.FeatureFlags.Has(kfmodels.KafkaClusterFeatureFlag) {
				return clusters.Cluster{}, operations.Operation{}, semerr.Authorization("operation is not allowed for this cloud")
			}

			createArgs, err := kf.convertMdbArgs(ctx, session, args)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			return kf.createClusterImpl(ctx, session, creator, createArgs)
		},
		clusterslogic.WithPermission(kfmodels.PermClustersCreate),
	)
}

func (kf *Kafka) CreateDataCloudCluster(ctx context.Context, args kafka.CreateDataCloudClusterArgs) (operations.Operation, error) {
	return kf.operator.Create(ctx, args.ProjectID,
		func(ctx context.Context, session sessions.Session, creator clusterslogic.Creator) (clusters.Cluster, operations.Operation, error) {
			createArgs, err := kf.convertDataCloudArgs(ctx, session, creator, args)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
			}

			return kf.createClusterImpl(ctx, session, creator, createArgs)
		},
		clusterslogic.WithPermission(kfmodels.PermClustersCreate),
	)
}

func (kf *Kafka) createClusterImpl(ctx context.Context, session sessions.Session, creator clusterslogic.Creator,
	args createClusterImplArgs) (clusters.Cluster, operations.Operation, error) {
	if err := args.Validate(); err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	_, subnets, err := kf.findSubnets(ctx, args.NetworkID, args.SubnetID)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to find subnets: %w", err)
	}

	cl, privKey, err := creator.CreateCluster(ctx, models.CreateClusterArgs{
		Name:               args.Name,
		ClusterType:        clusters.TypeKafka,
		Environment:        args.Environment,
		NetworkID:          args.NetworkID,
		FolderID:           session.FolderCoords.FolderID,
		Description:        args.Description,
		Labels:             args.Labels,
		HostGroupIDs:       args.HostGroupIDs,
		DeletionProtection: args.DeletionProtection,
		MaintenanceWindow:  args.MaintenanceWindow,
	})
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create cluster: %w", err)
	}

	// Create cluster pillar
	pillar, err := kf.makeClusterPillar(args, privKey)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
	}

	if len(args.SecurityGroupIDs) > 0 {
		args.SecurityGroupIDs = slices.DedupStrings(args.SecurityGroupIDs)
		// validate security groups after NetworkID validation, cause we validate SG above network
		if err := kf.compute.ValidateSecurityGroups(ctx, args.SecurityGroupIDs, args.NetworkID); err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}
	}

	err = kf.createKafkaHosts(ctx, session, creator, cl, pillar, subnets)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to add kafka hosts: %w", err)
	}

	err = kf.createZookeeperHosts(ctx, session, creator, cl, pillar, args.ZkZones, subnets)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to add zookeeper hosts: %w", err)
	}

	err = creator.AddClusterPillar(ctx, cl.ClusterID, cl.Revision, pillar)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
	}

	op, err := kf.tasks.CreateCluster(
		ctx,
		session,
		cl.ClusterID,
		cl.Revision,
		kfmodels.TaskTypeClusterCreate,
		kfmodels.OperationTypeClusterCreate,
		kfmodels.MetadataCreateCluster{},
		optional.String{},
		args.SecurityGroupIDs,
		kafkaService,
		searchAttributesExtractor,
	)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	return cl, op, nil
}

func (kf *Kafka) convertMdbArgs(ctx context.Context, session sessions.Session, args kafka.CreateMDBClusterArgs) (createClusterImplArgs, error) {
	oneHostMode := args.ConfigSpec.BrokersCount == 1 && len(args.ConfigSpec.ZoneID) == 1
	zkResourcesSpecified := args.ConfigSpec.ZooKeeper.Resources != (models.ClusterResources{})
	if oneHostMode && zkResourcesSpecified {
		return createClusterImplArgs{}, semerr.InvalidInputf("zookeeper resources should not be specified for single-broker cluster")
	}

	zonesWithHosts, zkZones := kf.buildZoneLists(args.ConfigSpec.ZoneID, args.HostGroupIDs, session)
	if len(args.HostGroupIDs) > 0 {
		err := kf.compute.ValidateHostGroups(ctx, args.HostGroupIDs, session.FolderCoords.FolderExtID, session.FolderCoords.CloudExtID, zonesWithHosts)
		if err != nil {
			return createClusterImplArgs{}, err
		}
	}

	result := createClusterImplArgs{
		Name:        args.Name,
		RegionID:    "",
		FolderExtID: args.FolderExtID,
		SyncTopics:  kf.cfg.Kafka.SyncTopics,
		CloudType:   environment.CloudTypeYandex,
		ConfigSpec: kfmodels.ClusterConfigSpec{
			Version:         args.ConfigSpec.Version,
			Kafka:           args.ConfigSpec.Kafka,
			ZooKeeper:       args.ConfigSpec.ZooKeeper,
			ZoneID:          args.ConfigSpec.ZoneID,
			BrokersCount:    args.ConfigSpec.BrokersCount,
			AssignPublicIP:  args.ConfigSpec.AssignPublicIP,
			UnmanagedTopics: args.ConfigSpec.UnmanagedTopics,
			SchemaRegistry:  args.ConfigSpec.SchemaRegistry,
			SyncTopics:      args.ConfigSpec.SyncTopics,
			Access: clusters.Access{
				DataTransfer: args.ConfigSpec.Access.DataTransfer,
				WebSQL:       args.ConfigSpec.Access.WebSQL,
				Serverless:   args.ConfigSpec.Access.Serverless,
			},
		},
		UserSpecs:          args.UserSpecs,
		TopicSpecs:         args.TopicSpecs,
		Environment:        args.Environment,
		IsPrestable:        kf.IsEnvPrestable(args.Environment),
		NetworkID:          args.NetworkID,
		SubnetID:           args.SubnetID,
		Description:        args.Description,
		Labels:             args.Labels,
		HostGroupIDs:       args.HostGroupIDs,
		DeletionProtection: args.DeletionProtection,
		MaintenanceWindow:  args.MaintenanceWindow,
		SecurityGroupIDs:   args.SecurityGroupIDs,
		ZkZones:            zkZones,
		OneHostMode:        oneHostMode,
		ZkScramEnabled:     false,
	}

	err := kf.initResourcesIfEmpty(&result.ConfigSpec.Kafka.Resources, hosts.RoleKafka)
	if err != nil {
		return createClusterImplArgs{}, err
	}

	err = kf.initResourcesIfEmpty(&result.ConfigSpec.ZooKeeper.Resources, hosts.RoleZooKeeper)
	if err != nil {
		return createClusterImplArgs{}, err
	}

	return result, nil
}

func (kf *Kafka) convertDataCloudArgs(ctx context.Context, session sessions.Session, creator clusterslogic.Creator,
	args kafka.CreateDataCloudClusterArgs) (createClusterImplArgs, error) {

	userPassword, err := crypto.GenerateRawPassword(kf.cryptoProvider, passwordGenLen, nil)
	if err != nil {
		return createClusterImplArgs{}, err
	}
	zkResources := models.ClusterResources{}
	err = kf.initResourcesIfEmpty(&zkResources, hosts.RoleZooKeeper)
	if err != nil {
		return createClusterImplArgs{}, err
	}

	defaultKafkaResources := models.ClusterResources{}
	err = kf.initResourcesIfEmpty(&defaultKafkaResources, hosts.RoleKafka)
	if err != nil {
		return createClusterImplArgs{}, err
	}
	args.ClusterSpec.Kafka.Resources.DiskTypeExtID = defaultKafkaResources.DiskTypeExtID

	oneHostMode := args.ClusterSpec.BrokersCount == 1 && args.ClusterSpec.ZoneCount == 1

	kafkaZones, zkZones, err := selectDataCloudKafkaAndZookeeperHostZones(ctx, session, creator, args.CloudType,
		args.RegionID, args.ClusterSpec.ZoneCount, oneHostMode)
	if err != nil {
		return createClusterImplArgs{}, err
	}

	if err := args.ClusterSpec.Access.ValidateAndSane(); err != nil {
		return createClusterImplArgs{}, err
	}

	if !args.NetworkID.Valid {
		network, err := compute.GetOrCreateNetwork(ctx, kf.compute, args.ProjectID, args.RegionID)
		if err != nil {
			return createClusterImplArgs{}, err
		}

		args.NetworkID.Set(network.ID)
	}

	return createClusterImplArgs{
		Name:        args.Name,
		RegionID:    args.RegionID,
		FolderExtID: args.ProjectID,
		SyncTopics:  true,
		CloudType:   args.CloudType,
		ConfigSpec: kfmodels.ClusterConfigSpec{
			Version: args.ClusterSpec.Version,
			Kafka:   args.ClusterSpec.Kafka,
			ZooKeeper: kfmodels.ZookeperConfigSpec{
				Resources: zkResources,
			},
			ZoneID:          kafkaZones,
			BrokersCount:    args.ClusterSpec.BrokersCount,
			AssignPublicIP:  false,
			UnmanagedTopics: true,
			SchemaRegistry:  false,
			SyncTopics:      true,
			Access:          args.ClusterSpec.Access,
			Encryption:      args.ClusterSpec.Encryption,
		},
		AdminUserName: dataCloudOwnerUserName,
		UserSpecs: []kfmodels.UserSpec{{
			Name:     dataCloudOwnerUserName,
			Password: userPassword,
			Permissions: []kfmodels.Permission{{
				TopicName:  "*",
				AccessRole: kfmodels.AccessRoleAdmin,
				Group:      "",
				Host:       "",
			}},
		}},
		TopicSpecs:         nil,
		Environment:        kf.cfg.SaltEnvs.Production,
		IsPrestable:        false,
		NetworkID:          args.NetworkID.String,
		SubnetID:           nil,
		Description:        args.Description,
		Labels:             nil,
		HostGroupIDs:       nil,
		DeletionProtection: false,
		MaintenanceWindow:  clusters.NewAnytimeMaintenanceWindow(),
		SecurityGroupIDs:   nil,
		ZkZones:            zkZones,
		OneHostMode:        oneHostMode,
		ZkScramEnabled:     true,
	}, nil
}

func (kf *Kafka) convertDataCloudArgsForEstimation(ctx context.Context, session sessions.Session, creator clusterslogic.Creator,
	args kafka.CreateDataCloudClusterArgs) (createClusterImplArgs, error) {

	zkResources := models.ClusterResources{}
	err := kf.initResourcesIfEmpty(&zkResources, hosts.RoleZooKeeper)
	if err != nil {
		return createClusterImplArgs{}, err
	}

	defaultKafkaResources := models.ClusterResources{}
	err = kf.initResourcesIfEmpty(&defaultKafkaResources, hosts.RoleKafka)
	if err != nil {
		return createClusterImplArgs{}, err
	}

	return createClusterImplArgs{
		ConfigSpec: kfmodels.ClusterConfigSpec{
			Kafka: kfmodels.KafkaConfigSpec{
				Resources: models.ClusterResources{
					ResourcePresetExtID: args.ClusterSpec.Kafka.Resources.ResourcePresetExtID,
					DiskSize:            args.ClusterSpec.Kafka.Resources.DiskSize,
					DiskTypeExtID:       defaultKafkaResources.DiskTypeExtID,
				},
			},
			ZooKeeper: kfmodels.ZookeperConfigSpec{
				Resources: zkResources,
			},
		},
	}, nil
}

func (kf *Kafka) makeClusterPillar(args createClusterImplArgs, privateKey []byte) (*kfpillars.Cluster, error) {
	pillar := kfpillars.NewCluster()

	pillar.Data.Kafka.AssignPublicIP = args.ConfigSpec.AssignPublicIP
	pillar.Data.Kafka.BrokersCount = args.ConfigSpec.BrokersCount
	pillar.Data.Kafka.ZoneID = args.ConfigSpec.ZoneID
	pillar.Data.Kafka.UnmanagedTopics = args.ConfigSpec.UnmanagedTopics
	pillar.Data.Kafka.SyncTopics = args.SyncTopics
	pillar.Data.Kafka.SchemaRegistry = args.ConfigSpec.SchemaRegistry
	pillar.Data.CloudType = args.CloudType
	pillar.Data.RegionID = args.RegionID
	pillar.SetAccess(args.ConfigSpec.Access)
	pillar.SetEncryption(args.ConfigSpec.Encryption)
	// One broker - no need for zk subcluster
	if args.OneHostMode {
		pillar.SetOneHostMode()
		pillar.Data.ZooKeeper.Config.DataDir = "/data/zookeeper"
	} else {
		pillar.SetHasZkSubcluster()
		pillar.Data.ZooKeeper.Config.DataDir = "/var/lib/zookeeper"
	}

	pillar.Data.Kafka.Resources = args.ConfigSpec.Kafka.Resources
	if pillar.Data.Kafka.Resources == (models.ClusterResources{}) {
		return nil, semerr.InvalidInput("kafka resources can not be empty")
	}
	pillar.Data.ZooKeeper.Resources = args.ConfigSpec.ZooKeeper.Resources
	if pillar.Data.ZooKeeper.Resources == (models.ClusterResources{}) {
		return nil, semerr.InvalidInput("zookeeper resources can not be empty")
	}

	err := pillar.SetVersion(args.ConfigSpec.Version)
	if err != nil {
		return nil, err
	}

	reflectutil.CopyStructFieldsStrict(&args.ConfigSpec.Kafka.Config, &pillar.Data.Kafka.Config)

	adminPassword, err := crypto.GenerateEncryptedPassword(kf.cryptoProvider, passwordGenLen, nil)
	if err != nil {
		return nil, err
	}
	pillar.Data.Kafka.AdminPassword = adminPassword

	monitorPassword, err := crypto.GenerateEncryptedPassword(kf.cryptoProvider, passwordGenLen, nil)
	if err != nil {
		return nil, err
	}
	pillar.Data.Kafka.MonitorPassword = monitorPassword

	if args.AdminUserName != "" {
		pillar.SetAdminUserName(args.AdminUserName)
	}

	if args.ZkScramEnabled {
		zkAdminPassword, err := crypto.GenerateEncryptedPassword(kf.cryptoProvider, passwordGenLen, nil)
		if err != nil {
			return nil, err
		}
		pillar.Data.ZooKeeper.ScramAdminPassword = &zkAdminPassword
		pillar.Data.ZooKeeper.ScramAuthEnabled = true
	}

	privateKeyEnc, err := kf.cryptoProvider.Encrypt(privateKey)
	if err != nil {
		return nil, err
	}
	pillar.Data.ClusterPrivateKey = privateKeyEnc

	if kf.cfg.E2E.IsClusterE2E(args.Name, args.FolderExtID) {
		pillar.Data.MDBMetrics = pillars.NewDisabledMDBMetrics()
		pillar.Data.Billing = pillars.NewDisabledBilling()
		pillar.Data.UseYASMAgent = new(bool)
		pillar.Data.SuppressExternalYASMAgent = true
		pillar.Data.ShipLogs = new(bool)
		pillar.Data.MDBHealth = pillars.NewMDBHealthWithDisabledAggregate()
	}

	for _, topic := range args.TopicSpecs {
		topicData := kfpillars.TopicData{
			Name:              topic.Name,
			Partitions:        topic.Partitions,
			ReplicationFactor: topic.ReplicationFactor,
		}
		reflectutil.CopyStructFieldsStrict(&topic.Config, &topicData.Config)
		err := pillar.AddTopic(topicData)
		if err != nil {
			return nil, xerrors.Errorf("failed to add topic %q operation: %w", topic.Name, err)
		}
	}

	for _, user := range args.UserSpecs {
		perms := make([]kfpillars.PermissionsData, 0, len(user.Permissions))
		for _, permisison := range user.Permissions {
			perms = append(perms, kfpillars.PermissionsData{
				TopicName: permisison.TopicName,
				Role:      string(permisison.AccessRole),
				Group:     permisison.Group,
				Host:      permisison.Host,
			})
		}

		err = kfmodels.ValidateNonEmptyPassword(user.Password)
		if err != nil {
			return nil, err
		}

		passwordEncrypted, err := kf.cryptoProvider.Encrypt([]byte(user.Password.Unmask()))
		if err != nil {
			return nil, xerrors.Errorf("failed to encrypt user %q password: %w", user.Name, err)
		}
		err = pillar.AddUser(kfpillars.UserData{
			Name:        user.Name,
			Password:    passwordEncrypted,
			Permissions: perms,
		})
		if err != nil {
			return nil, xerrors.Errorf("failed to add user %q operation: %w", user.Name, err)
		}
	}
	return pillar, nil
}

func (kf *Kafka) createKafkaHosts(ctx context.Context, session sessions.Session, creator clusterslogic.Creator,
	cl clusters.Cluster, pillar *kfpillars.Cluster, subnets []networkProvider.Subnet) error {
	kafkaSubCluster, err := creator.CreateSubCluster(ctx, models.CreateSubClusterArgs{
		ClusterID: cl.ClusterID,
		Name:      kfmodels.KafkaSubClusterName,
		Roles:     []hosts.Role{hosts.RoleKafka},
		Revision:  cl.Revision,
	})
	if err != nil {
		return xerrors.Errorf("failed to create kafka subcluster: %w", err)
	}

	kafkaResolvedHostGroup, err := kf.makeKafkaClusterHostGroups(ctx, creator, session, pillar.Data.Kafka, !pillar.HasZkSubcluster())
	if err != nil {
		return xerrors.Errorf("failed to create host groups: %w", err)
	}
	if (kf.cfg.EnvironmentVType == environment.VTypeCompute) && (kafkaResolvedHostGroup.NewResourcePreset.Generation == 1) {
		return semerr.InvalidInput("creating hosts on Intel Broadwell is impossible")
	}

	var addHostsArgs []models.AddHostArgs
	kafkaHostArgs := models.AddHostArgs{
		SubClusterID:   kafkaSubCluster.SubClusterID,
		SpaceLimit:     pillar.Data.Kafka.Resources.DiskSize,
		DiskTypeExtID:  pillar.Data.Kafka.Resources.DiskTypeExtID,
		Revision:       cl.Revision,
		ClusterID:      cl.ClusterID,
		AssignPublicIP: pillar.Data.Kafka.AssignPublicIP,
	}

	addHostsArgs, err = kf.addKafkaHosts(ctx, session, creator, addHostsArgs, pillar.Data.CloudType, subnets,
		kafkaResolvedHostGroup.TargetResourcePreset(), kafkaHostArgs, pillar, kafkaResolvedHostGroup.HostsToAdd, nil)
	if err != nil {
		return xerrors.Errorf("failed to add broker hosts: %w", err)
	}

	if _, err = creator.AddHosts(ctx, addHostsArgs); err != nil {
		return xerrors.Errorf("failed to add hosts: %w", err)
	}

	return nil
}

func (kf *Kafka) createZookeeperHosts(ctx context.Context, session sessions.Session, creator clusterslogic.Creator,
	cl clusters.Cluster, pillar *kfpillars.Cluster, zkZones []string, subnets []networkProvider.Subnet) error {

	if pillar.HasZkSubcluster() {
		zkResolvedHostGroup, err := kf.makeZookeeperClusterHostGroups(ctx, creator, session, zkZones, pillar.Data.ZooKeeper.Resources)
		if err != nil {
			return err
		}

		// Zookeeper subcluster creation
		var zkAddSubClusterArgs = models.CreateSubClusterArgs{
			ClusterID: cl.ClusterID,
			Name:      kfmodels.ZooKeeperSubClusterName,
			Roles:     []hosts.Role{hosts.RoleZooKeeper},
			Revision:  cl.Revision,
		}
		zkSubCluster, err := creator.CreateSubCluster(ctx, zkAddSubClusterArgs)
		if err != nil {
			return xerrors.Errorf("failed to create zk subcluster: %w", err)
		}

		zkHostArgs := models.AddHostArgs{
			SubClusterID:  zkSubCluster.SubClusterID,
			SpaceLimit:    pillar.Data.ZooKeeper.Resources.DiskSize,
			DiskTypeExtID: pillar.Data.ZooKeeper.Resources.DiskTypeExtID,
			Revision:      cl.Revision,
			ClusterID:     cl.ClusterID,
		}
		var addHostsArgs []models.AddHostArgs
		addHostsArgs, err = kf.addZkHosts(ctx, session, creator, addHostsArgs, pillar.Data.CloudType, subnets,
			zkResolvedHostGroup.TargetResourcePreset(), zkHostArgs, pillar, zkZones)
		if err != nil {
			return xerrors.Errorf("failed to add zk hosts: %w", err)
		}

		if _, err = creator.AddHosts(ctx, addHostsArgs); err != nil {
			return xerrors.Errorf("failed to add hosts: %w", err)
		}
	} else {
		// Need to add only host in the zk pillar
		nodesCounter := 0
		pillar.Data.ZooKeeper.Resources = models.ClusterResources{}
		for _, node := range pillar.Data.Kafka.Nodes {
			nodesCounter++
			err := pillar.AddZooKeeperNode(node.FQDN, nodesCounter)
			if err != nil {
				return xerrors.Errorf("failed to add zookeeper node: %w", err)
			}
		}
	}

	return nil
}

func (kf *Kafka) makeKafkaClusterHostGroups(ctx context.Context, creator clusterslogic.Creator, session sessions.Session,
	kafka kfpillars.KafkaData, oneHostMode bool) (*clusterslogic.ResolvedHostGroup, error) {
	kafkaHostGroup := clusterslogic.HostGroup{
		Role:                   hosts.RoleKafka,
		NewResourcePresetExtID: optional.NewString(kafka.Resources.ResourcePresetExtID),
		DiskTypeExtID:          kafka.Resources.DiskTypeExtID,
		NewDiskSize:            optional.NewInt64(kafka.Resources.DiskSize),
		HostsToAdd:             make([]clusterslogic.ZoneHosts, 0, len(kafka.ZoneID)),
	}
	for _, zoneID := range kafka.ZoneID {
		kafkaHostGroup.HostsToAdd = append(kafkaHostGroup.HostsToAdd, clusterslogic.ZoneHosts{ZoneID: zoneID, Count: kafka.BrokersCount})
	}

	resolvedHostGroups, _, err := creator.ValidateResources(ctx, session, clusters.TypeKafka, kafkaHostGroup)
	if err != nil {
		return nil, err
	}
	var kafkaResolvedHostGroup clusterslogic.ResolvedHostGroup
	if oneHostMode {
		kafkaResolvedHostGroup = resolvedHostGroups.Single()
	} else {
		kafkaResolvedHostGroup = resolvedHostGroups.MustByHostRole(hosts.RoleKafka)
	}
	return &kafkaResolvedHostGroup, nil
}

func (kf *Kafka) makeZookeeperClusterHostGroups(ctx context.Context, creator clusterslogic.Creator, session sessions.Session,
	zkZones []string, zkResources models.ClusterResources) (*clusterslogic.ResolvedHostGroup, error) {

	zkHostGroup := clusterslogic.HostGroup{
		Role:                   hosts.RoleZooKeeper,
		NewResourcePresetExtID: optional.NewString(zkResources.ResourcePresetExtID),
		DiskTypeExtID:          zkResources.DiskTypeExtID,
		NewDiskSize:            optional.NewInt64(zkResources.DiskSize),
		HostsToAdd:             make([]clusterslogic.ZoneHosts, 0, len(kf.cfg.Kafka.ZooKeeperZones)),
		SkipValidations: clusterslogic.SkipValidations{
			DecommissionedZone: true,
		},
	}
	for _, zoneID := range zkZones {
		zkHostGroup.HostsToAdd = append(zkHostGroup.HostsToAdd, clusterslogic.ZoneHosts{ZoneID: zoneID, Count: 1})
	}

	resolvedHostGroups, _, err := creator.ValidateResources(
		ctx,
		session,
		clusters.TypeKafka,
		zkHostGroup,
	)
	if err != nil {
		return nil, err
	}

	zkResolvedHostGroup := resolvedHostGroups.MustByHostRole(hosts.RoleZooKeeper)
	return &zkResolvedHostGroup, nil
}

func (kf *Kafka) addZkHosts(ctx context.Context, session sessions.Session, generator fqdnGenerator, addHostsArgs []models.AddHostArgs,
	cloudType environment.CloudType, subnets []networkProvider.Subnet, preset resources.Preset, commonArgs models.AddHostArgs,
	pillar *kfpillars.Cluster, zones []string) ([]models.AddHostArgs, error) {

	zoneHostsList := clusterslogic.ZoneHostsList{}
	for _, zoneID := range zones {
		zoneHostsList = zoneHostsList.Add(clusterslogic.ZoneHostsList{clusterslogic.ZoneHosts{ZoneID: zoneID, Count: 1}})
	}

	fqdnsMap, err := generator.GenerateSemanticFQDNs(cloudType, clusters.TypeKafka, zoneHostsList,
		nil, "zk", commonArgs.ClusterID, preset.VType, intcompute.Ubuntu)
	if err != nil {
		return nil, err
	}

	var hostIdx = 0
	for _, zoneHosts := range zoneHostsList {
		subnet, err := kf.compute.PickSubnet(ctx, subnets, preset.VType, zoneHosts.ZoneID, commonArgs.AssignPublicIP, optional.String{}, session.FolderCoords.FolderExtID)
		if err != nil {
			return nil, err
		}

		for _, fqdn := range fqdnsMap[zoneHosts.ZoneID] {
			addHostArgs := models.AddHostArgs{
				SubClusterID:     commonArgs.SubClusterID,
				ResourcePresetID: preset.ID,
				SpaceLimit:       commonArgs.SpaceLimit,
				DiskTypeExtID:    commonArgs.DiskTypeExtID,
				Revision:         commonArgs.Revision,
				FQDN:             fqdn,
				ZoneID:           zoneHosts.ZoneID,
				AssignPublicIP:   false,
				ClusterID:        commonArgs.ClusterID,
				SubnetID:         subnet.ID,
			}
			addHostsArgs = append(addHostsArgs, addHostArgs)
			hostIdx = hostIdx + 1
			err = pillar.AddZooKeeperNode(fqdn, hostIdx)
			if err != nil {
				return nil, xerrors.Errorf("failed to add zookeeper node: %w", err)
			}
		}
	}
	return addHostsArgs, nil
}

func (kf *Kafka) buildZoneLists(kafkaZones []string, hostGroupIDs []string, session sessions.Session) ([]string, []string) {
	zkZones := kf.cfg.Kafka.ZooKeeperZones
	zkCount := len(zkZones)
	var zonesWithHosts []string
	if len(hostGroupIDs) > 0 && session.FeatureFlags.Has(kfmodels.KafkaAllowNonHAOnHGFeatureFlag) {
		// By default, we host one zookeeper host in each of the 3 availability zones.
		// When using dedicated servers, this means that the user must have at least one dedicated server
		// (and, accordingly, a host group) in each of the availability zones, even if he places brokers
		// in one or two zones.
		// This can be costly for the user. In addition, it makes testing more difficult for us.
		// Therefore, if this feature flag is set, then we place the zookeeper hosts in the same zones
		// in which the user requested the creation of brokers. In this case, more than one zookeeper host
		// can be placed in one zone.
		zkZones = []string{}
		for i := 0; i < zkCount; i++ {
			zkZones = append(zkZones, kafkaZones[i%len(kafkaZones)])
		}
		zonesWithHosts = kafkaZones
	} else {
		zonesWithHosts = make([]string, 0, len(kafkaZones)+len(zkZones))
		zonesWithHosts = append(zonesWithHosts, kafkaZones...)
		zonesWithHosts = append(zonesWithHosts, zkZones...)
		zonesWithHosts = slices.DedupStrings(zonesWithHosts)
	}
	return zonesWithHosts, zkZones
}
