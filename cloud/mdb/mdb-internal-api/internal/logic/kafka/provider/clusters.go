package provider

import (
	"context"
	"sort"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/compute"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	taskslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/provider/internal/kfpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	kafkaService = "managed-kafka"
)

func (kf *Kafka) MDBCluster(ctx context.Context, cid string) (kfmodels.MDBCluster, error) {
	var cluster kfmodels.MDBCluster
	if err := kf.operator.ReadCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			cl, err := reader.ClusterExtendedByClusterID(ctx, cid, clusters.TypeKafka, models.VisibilityVisible, session)
			if err != nil {
				return err
			}

			pillar, err := getPillarFromJSON(cl.Pillar)
			if err != nil {
				return err
			}

			cluster = kfmodels.MDBCluster{
				ClusterExtended: cl,
				Config: kfmodels.MDBClusterSpec{
					Kafka: kfmodels.KafkaConfigSpec{
						Resources: models.ClusterResources{
							ResourcePresetExtID: pillar.Data.Kafka.Resources.ResourcePresetExtID,
							DiskTypeExtID:       pillar.Data.Kafka.Resources.DiskTypeExtID,
							DiskSize:            pillar.Data.Kafka.Resources.DiskSize,
						},
					},
					ZoneID:          pillar.Data.Kafka.ZoneID,
					BrokersCount:    pillar.Data.Kafka.BrokersCount,
					AssignPublicIP:  pillar.Data.Kafka.AssignPublicIP,
					Version:         pillar.Data.Kafka.Version,
					UnmanagedTopics: pillar.Data.Kafka.UnmanagedTopics,
					SchemaRegistry:  pillar.Data.Kafka.SchemaRegistry,
					SyncTopics:      pillar.Data.Kafka.SyncTopics,
					Access: kfmodels.Access{
						DataTransfer: pillars.MapPtrBoolToOptionalBool(pillar.Data.Access.DataTransfer),
					},
				},
			}
			if pillar.HasZkSubcluster() {
				cluster.Config.ZooKeeper = kfmodels.ZookeperConfigSpec{
					Resources: models.ClusterResources{
						ResourcePresetExtID: pillar.Data.ZooKeeper.Resources.ResourcePresetExtID,
						DiskTypeExtID:       pillar.Data.ZooKeeper.Resources.DiskTypeExtID,
						DiskSize:            pillar.Data.ZooKeeper.Resources.DiskSize,
					},
				}
			}
			reflectutil.CopyStructFieldsStrict(&pillar.Data.Kafka.Config, &cluster.Config.Kafka.Config)
			return nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersGet),
	); err != nil {
		return kfmodels.MDBCluster{}, err
	}

	return cluster, nil
}

func (kf *Kafka) DataCloudCluster(ctx context.Context, cid string, sensitive bool) (kfmodels.DataCloudCluster, error) {
	result := kfmodels.DataCloudCluster{}

	if err := kf.operator.ReadCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			cluster, err := reader.ClusterExtendedByClusterID(ctx, cid, clusters.TypeKafka, models.VisibilityVisible, session)
			if err != nil {
				return err
			}

			result, err = kf.getDataCloudCluster(ctx, cluster, sensitive)
			if err != nil {
				return err
			}

			return nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersGet),
	); err != nil {
		return kfmodels.DataCloudCluster{}, err
	}

	return result, nil
}

func (kf *Kafka) getDataCloudCluster(ctx context.Context, cluster clusters.ClusterExtended,
	sensitive bool) (kfmodels.DataCloudCluster, error) {
	pillar, err := getPillarFromJSON(cluster.Pillar)
	if err != nil {
		return kfmodels.DataCloudCluster{}, err
	}

	connectionInfo, privateConnectionInfo, err := kf.getDataCloudConnectionInfo(ctx, pillar, cluster.ClusterID, sensitive)
	if err != nil {
		return kfmodels.DataCloudCluster{}, err
	}

	return kfmodels.DataCloudCluster{
		ClusterExtended:       cluster,
		CloudType:             pillar.Data.CloudType,
		Version:               pillar.Data.Kafka.Version,
		RegionID:              pillar.Data.RegionID,
		ConnectionInfo:        connectionInfo,
		PrivateConnectionInfo: privateConnectionInfo,
		Resources: kfmodels.DataCloudResources{
			ResourcePresetID: optional.NewString(pillar.Data.Kafka.Resources.ResourcePresetExtID),
			DiskSize:         optional.NewInt64(pillar.Data.Kafka.Resources.DiskSize),
			BrokerCount:      optional.NewInt64(pillar.Data.Kafka.BrokersCount),
			ZoneCount:        optional.NewInt64(int64(len(pillar.Data.Kafka.ZoneID))),
		},
		Access:     pillar.Data.Access.ToModel(),
		Encryption: pillar.Data.Encryption.ToModel(),
	}, nil
}

func (kf *Kafka) getDataCloudConnectionInfo(ctx context.Context, pillar *kfpillars.Cluster, cid string, sensitive bool) (
	kfmodels.ConnectionInfo, kfmodels.PrivateConnectionInfo, error) {
	adminUserName := pillar.GetAdminUserName()
	user, found := pillar.Data.Kafka.Users[adminUserName]
	if !found {
		return kfmodels.ConnectionInfo{}, kfmodels.PrivateConnectionInfo{}, xerrors.Errorf("can't find user: %s", adminUserName)
	}

	nodes := getConnectionNodes(pillar)

	connectionInfo := kfmodels.ConnectionInfo{
		ConnectionString: getConnectionString(nodes),
		User:             user.Name,
	}
	privateConnectionInfo := kfmodels.PrivateConnectionInfo{
		ConnectionString: getPrivateConnectionString(nodes),
		User:             user.Name,
	}
	dataCloudAdminPasswordPath := []string{"data", "kafka", "users", adminUserName, "password"}
	if sensitive {
		password, err := kf.pillarSecrets.GetClusterPillarSecret(ctx, cid, dataCloudAdminPasswordPath)
		if err != nil {
			if !semerr.IsAuthorization(err) {
				return kfmodels.ConnectionInfo{}, kfmodels.PrivateConnectionInfo{}, err
			}
		}
		connectionInfo.Password = password
		privateConnectionInfo.Password = password
	}

	return connectionInfo, privateConnectionInfo, nil
}

func getConnectionNodes(pillar *kfpillars.Cluster) []kfpillars.KafkaNodeData {
	var nodes []string
	for fqdn := range pillar.Data.Kafka.Nodes {
		nodes = append(nodes, fqdn)
	}
	sort.Strings(nodes)

	// Getting two first nodes from different zones if we have multiple zones
	result := make([]kfpillars.KafkaNodeData, 0, 2)
	zone := "nonexistent"
	for _, fqdn := range nodes {
		node, ok := pillar.Data.Kafka.Nodes[fqdn]
		if !ok {
			continue
		}
		// Skip already added zone if we have more than one
		if zone == node.Rack && len(pillar.Data.Kafka.ZoneID) > 1 {
			continue
		}
		zone = node.Rack
		result = append(result, node)
		if len(result) == 2 {
			break
		}
	}
	return result
}

func getPrivateFQDN(fqdn string, domainConfig api.DomainConfig) string {
	return strings.Replace(fqdn, domainConfig.Public, domainConfig.Private, -1)
}

func getConnectionString(connectionNodes []kfpillars.KafkaNodeData) string {
	var result []string
	for _, node := range connectionNodes {
		result = append(result, node.FQDN+":9091")
	}
	return strings.Join(result, ",")
}

func getPrivateConnectionString(connectionNodes []kfpillars.KafkaNodeData) string {
	var result []string
	for _, node := range connectionNodes {
		result = append(result, node.PrivateFQDN+":19091")
	}
	return strings.Join(result, ",")
}

func (kf *Kafka) MDBClusters(ctx context.Context, folderExtID string, limit, offset int64) ([]kfmodels.MDBCluster, error) {
	var res []kfmodels.MDBCluster
	if err := kf.operator.ReadOnFolder(ctx, folderExtID,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			if limit <= 0 {
				limit = 100
			}

			cls, err := reader.ClustersExtended(
				ctx,
				models.ListClusterArgs{
					ClusterType: clusters.TypeKafka,
					FolderID:    session.FolderCoords.FolderID,
					Limit:       optional.NewInt64(limit),
					Offset:      offset,
					Visibility:  models.VisibilityVisible,
				},
				session,
			)
			if err != nil {
				return xerrors.Errorf("failed to retrieve clusters: %w", err)
			}

			res = make([]kfmodels.MDBCluster, 0, len(cls))
			for _, cl := range cls {
				res = append(res, kfmodels.MDBCluster{
					ClusterExtended: cl,
				})
			}
			return nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersGet),
	); err != nil {
		return nil, err
	}

	return res, nil
}

func (kf *Kafka) DataCloudClusters(ctx context.Context, folderExtID string, limit, offset int64) ([]kfmodels.DataCloudCluster, error) {
	var res []kfmodels.DataCloudCluster
	if err := kf.operator.ReadOnFolder(ctx, folderExtID,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			if limit <= 0 {
				limit = 100
			}

			allClusters, err := reader.ClustersExtended(
				ctx,
				models.ListClusterArgs{
					ClusterType: clusters.TypeKafka,
					FolderID:    session.FolderCoords.FolderID,
					Limit:       optional.NewInt64(limit),
					Offset:      offset,
					Visibility:  models.VisibilityVisible,
				},
				session,
			)
			if err != nil {
				return xerrors.Errorf("failed to retrieve clusters: %w", err)
			}

			res = make([]kfmodels.DataCloudCluster, 0, len(allClusters))
			for _, currentCluster := range allClusters {
				dataCloudCluster, err := kf.getDataCloudCluster(ctx, currentCluster, false)
				if err != nil {
					return err
				}

				res = append(res, dataCloudCluster)
			}
			return nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersGet),
	); err != nil {
		return nil, err
	}

	return res, nil
}

func (kf *Kafka) findSubnets(ctx context.Context, networkID string, subnetID []string) (networkProvider.Network, []networkProvider.Subnet, error) {
	// in porto return empty subnet list
	if networkID == "" {
		return networkProvider.Network{}, nil, nil
	}

	network, subnets, err := kf.compute.NetworkAndSubnets(ctx, networkID)
	if err != nil {
		return networkProvider.Network{}, nil, err
	}

	// No subnets provided, no need to filter
	if len(subnetID) == 0 {
		return network, subnets, nil
	}

	subnetsIDSet := make(map[string]struct{}, len(subnetID))
	for _, subnetID := range subnetID {
		subnetsIDSet[subnetID] = struct{}{}
	}
	subnetsFiltered := make([]networkProvider.Subnet, 0)
	for _, subnet := range subnets {
		if _, ok := subnetsIDSet[subnet.ID]; ok {
			subnetsFiltered = append(subnetsFiltered, subnet)
		}
	}

	// This means that some of the subnets are not found in network. We need to find which one
	if len(subnetsFiltered) < len(subnetID) {
		// Making map of subnets that were found in network
		existsSubnetsIDSet := make(map[string]struct{}, len(subnets))
		for _, subnet := range subnets {
			existsSubnetsIDSet[subnet.ID] = struct{}{}
		}
		for _, subnetID := range subnetID {
			if _, ok := existsSubnetsIDSet[subnetID]; !ok {
				return networkProvider.Network{}, nil, semerr.InvalidInputf("subnet %q not found in network %q", subnetID, networkID)
			}
		}
	}

	return network, subnetsFiltered, nil
}

type fqdnGenerator interface {
	GenerateSemanticFQDNs(cloudType environment.CloudType, clusterType clusters.Type, zonesToCreate clusterslogic.ZoneHostsList,
		zonesCurrent clusterslogic.ZoneHostsList, shardName string, cid string, vtype environment.VType, platform compute.Platform,
	) (map[string][]string, error)
}

func maxBrokerID(kafkaData *kfpillars.KafkaData) (brokerID int) {
	for _, node := range kafkaData.Nodes {
		if brokerID < node.ID {
			brokerID = node.ID
		}
	}
	return
}

func (kf *Kafka) addKafkaHosts(ctx context.Context, session sessions.Session, generator fqdnGenerator, addHostsArgs []models.AddHostArgs, cloudType environment.CloudType,
	subnets []networkProvider.Subnet, preset resources.Preset, commonArgs models.AddHostArgs, pillar *kfpillars.Cluster,
	newHosts clusterslogic.ZoneHostsList, currentHosts clusterslogic.ZoneHostsList) ([]models.AddHostArgs, error) {

	fqdnsMap, err := generator.GenerateSemanticFQDNs(cloudType, clusters.TypeKafka, newHosts,
		currentHosts, "", commonArgs.ClusterID, preset.VType, compute.Ubuntu)
	if err != nil {
		return nil, err
	}

	zones := make([]string, 0)
	for k := range fqdnsMap {
		zones = append(zones, k)
	}
	sort.Strings(zones)

	nodesCounter := maxBrokerID(&pillar.Data.Kafka)
	for _, zoneID := range zones {
		subnet, err := kf.compute.PickSubnet(ctx, subnets, preset.VType, zoneID, commonArgs.AssignPublicIP, optional.String{}, session.FolderCoords.FolderExtID)
		if err != nil {
			return nil, err
		}
		for _, fqdn := range fqdnsMap[zoneID] {
			nodesCounter++
			addHostArgs := models.AddHostArgs{
				SubClusterID:     commonArgs.SubClusterID,
				ResourcePresetID: preset.ID,
				SpaceLimit:       commonArgs.SpaceLimit,
				DiskTypeExtID:    commonArgs.DiskTypeExtID,
				Revision:         commonArgs.Revision,
				FQDN:             fqdn,
				ZoneID:           zoneID,
				AssignPublicIP:   commonArgs.AssignPublicIP,
				ClusterID:        commonArgs.ClusterID,
				SubnetID:         subnet.ID,
			}
			addHostsArgs = append(addHostsArgs, addHostArgs)
			err = pillar.AddKafkaNode(kfpillars.KafkaNodeData{
				ID:          nodesCounter,
				FQDN:        fqdn,
				PrivateFQDN: getPrivateFQDN(fqdn, kf.domainConfig),
				Rack:        zoneID,
			})
			if err != nil {
				return nil, xerrors.Errorf("failed to add kafka pillar: %w", err)
			}
		}
	}
	return addHostsArgs, nil
}

func (kf *Kafka) initResourcesIfEmpty(resources *models.ClusterResources, role hosts.Role) error {
	if *resources != (models.ClusterResources{}) {
		return nil
	}
	defaultResources, err := kf.cfg.GetDefaultResources(clusters.TypeKafka, role)
	if err != nil {
		return xerrors.Errorf("failed to get default resources for %s host with role %s: %w", clusters.TypeKafka.String(), role.String(), err)
	}

	resources.ResourcePresetExtID = defaultResources.ResourcePresetExtID
	resources.DiskTypeExtID = defaultResources.DiskTypeExtID
	resources.DiskSize = defaultResources.DiskSize

	return nil
}

func searchAttributesExtractor(cluster clusterslogic.Cluster) (map[string]interface{}, error) {
	pillar, err := getPillarFromCluster(cluster)
	if err != nil {
		return nil, err
	}
	searchAttributes := make(map[string]interface{})

	topics := make([]string, 0, len(pillar.Data.Kafka.Topics))
	for t := range pillar.Data.Kafka.Topics {
		topics = append(topics, t)
	}
	sort.Strings(topics)
	searchAttributes["topics"] = topics

	users := make([]string, 0, len(pillar.Data.Kafka.Users))
	for u := range pillar.Data.Kafka.Users {
		users = append(users, u)
	}
	sort.Strings(users)
	searchAttributes["users"] = users

	return searchAttributes, nil
}

func (kf *Kafka) DeleteCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return kf.operator.Delete(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, cluster clusterslogic.Cluster, _ clusterslogic.Reader) (operations.Operation, error) {
			var s3Buckets taskslogic.DeleteClusterS3Buckets

			op, err := kf.tasks.DeleteCluster(
				ctx,
				session,
				cluster.ClusterID,
				cluster.Revision,
				taskslogic.DeleteClusterTaskTypes{
					Delete:   kfmodels.TaskTypeClusterDelete,
					Metadata: kfmodels.TaskTypeClusterDeleteMetadata,
				},
				kfmodels.OperationTypeClusterDelete,
				s3Buckets,
			)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := kf.search.StoreDocDelete(
				ctx,
				kafkaService,
				session.FolderCoords.FolderExtID,
				session.FolderCoords.CloudExtID,
				op,
			); err != nil {
				return operations.Operation{}, err
			}
			return op, nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersDelete),
	)
}

func (kf *Kafka) StartCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return kf.operator.ModifyOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, _ clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			op, err := kf.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      kfmodels.TaskTypeClusterStart,
					OperationType: kfmodels.OperationTypeClusterStart,
					Revision:      cluster.Revision,
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}
			return op, nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersStart),
	)
}

func (kf *Kafka) StopCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return kf.operator.ModifyOnCluster(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, _ clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			op, err := kf.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      kfmodels.TaskTypeClusterStop,
					OperationType: kfmodels.OperationTypeClusterStop,
					Revision:      cluster.Revision,
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}
			return op, nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersStop),
	)
}

func (kf *Kafka) ListHosts(ctx context.Context, cid string, limit, offset int64) ([]hosts.HostExtended, error) {
	res, _, _, err := clusterslogic.ListHosts(ctx, kf.operator, cid, clusters.TypeKafka, limit, offset)
	return res, err
}

func (kf *Kafka) EstimateCreateCluster(ctx context.Context, args kafka.CreateMDBClusterArgs) (console.BillingEstimate, error) {
	res := console.BillingEstimate{}
	err := kf.operator.ReadOnFolder(ctx, args.FolderExtID,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			spec := args.ConfigSpec
			kafkaHostsCount := len(spec.ZoneID) * int(spec.BrokersCount)
			zkHostsCount := 0
			if kafkaHostsCount > 1 {
				zkHostsCount = len(kf.cfg.Kafka.ZooKeeperZones)
			}
			hostsCount := kafkaHostsCount + zkHostsCount
			hostBillingSpecs := make([]clusterslogic.HostBillingSpec, 0, hostsCount)

			for i := 0; i < kafkaHostsCount; i++ {
				kafkaResources := spec.Kafka.Resources
				err := kf.initResourcesIfEmpty(&kafkaResources, hosts.RoleKafka)
				if err != nil {
					return err
				}
				hostBillingSpecs = append(hostBillingSpecs, clusterslogic.HostBillingSpec{
					HostRole:         hosts.RoleKafka,
					ClusterResources: kafkaResources,
					AssignPublicIP:   spec.AssignPublicIP,
					OnDedicatedHost:  len(args.HostGroupIDs) > 0,
				})
			}

			for i := 0; i < zkHostsCount; i++ {
				zkResources := spec.ZooKeeper.Resources
				err := kf.initResourcesIfEmpty(&zkResources, hosts.RoleZooKeeper)
				if err != nil {
					return err
				}
				hostBillingSpecs = append(hostBillingSpecs, clusterslogic.HostBillingSpec{
					HostRole:         hosts.RoleZooKeeper,
					ClusterResources: zkResources,
					AssignPublicIP:   false,
					OnDedicatedHost:  len(args.HostGroupIDs) > 0,
				})
			}

			var err error
			res, err = reader.EstimateBilling(ctx, args.FolderExtID, clusters.TypeKafka, hostBillingSpecs, environment.CloudTypeYandex)
			return err
		},
		clusterslogic.WithPermission(kfmodels.PermClustersGet),
	)
	return res, err
}

func (kf *Kafka) EstimateCreateDCCluster(ctx context.Context, args kafka.CreateDataCloudClusterArgs) (console.BillingEstimate, error) {
	var (
		res console.BillingEstimate
		err error
	)

	err = kf.operator.FakeCreate(ctx, args.ProjectID,
		func(ctx context.Context, session sessions.Session, creator clusterslogic.Creator) error {
			var (
				clusterArgs                   createClusterImplArgs
				kafkaHostsCount, zkHostsCount int64
			)

			clusterArgs, err = kf.convertDataCloudArgsForEstimation(ctx, session, creator, args)
			if err != nil {
				return err
			}

			kafkaHostsCount = args.ClusterSpec.BrokersCount * args.ClusterSpec.ZoneCount
			if kafkaHostsCount > 1 {
				zkHostsCount = int64(len(kf.cfg.Kafka.ZooKeeperZones))
			}

			hostBillingSpecs := make([]clusterslogic.HostBillingSpec, 0, kafkaHostsCount+zkHostsCount)

			for i := int64(0); i < kafkaHostsCount; i++ {
				hostBillingSpecs = append(hostBillingSpecs, clusterslogic.HostBillingSpec{
					HostRole:         hosts.RoleKafka,
					ClusterResources: clusterArgs.ConfigSpec.Kafka.Resources,
				})
			}
			for i := int64(0); i < zkHostsCount; i++ {
				hostBillingSpecs = append(hostBillingSpecs, clusterslogic.HostBillingSpec{
					HostRole:         hosts.RoleZooKeeper,
					ClusterResources: clusterArgs.ConfigSpec.ZooKeeper.Resources,
				})
			}

			res, err = creator.EstimateBilling(ctx, args.ProjectID, clusters.TypeKafka, hostBillingSpecs, environment.CloudTypeAWS)

			for i := 0; i < len(res.Metrics); i++ {
				res.Metrics[i].Tags.CloudRegion = args.RegionID
				res.Metrics[i].Tags.CloudProvider = string(args.CloudType)
			}

			return err
		},
	)
	return res, err
}

func ValidateCluster(cluster metadb.Cluster) error {
	pillar, err := getPillarFromJSON(cluster.Pillar)
	if err != nil {
		return err
	}

	return pillar.Validate()
}

func (kf *Kafka) MoveCluster(ctx context.Context, cid, destinationFolderID string) (operations.Operation, error) {
	return kf.operator.MoveCluster(ctx, cid, destinationFolderID, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			err := modifier.UpdateClusterFolder(ctx, cluster, destinationFolderID)
			if err != nil {
				return operations.Operation{}, err
			}

			op, err := kf.tasks.MoveCluster(
				ctx,
				session,
				cid,
				cluster.Revision,
				kfmodels.OperationTypeClusterMove,
				kfmodels.MetadataMoveCluster{
					SourceFolderID:      session.FolderCoords.FolderExtID,
					DestinationFolderID: destinationFolderID,
				},
				func(options *taskslogic.MoveClusterOptions) {
					options.TaskArgs = map[string]interface{}{}
					options.SrcFolderTaskType = kfmodels.TaskTypeClusterMove
					options.DstFolderTaskType = kfmodels.TaskTypeClusterMoveNoOp
					options.SrcFolderCoords = session.FolderCoords
					options.DstFolderExtID = destinationFolderID
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		})
}

func (kf *Kafka) RescheduleMaintenance(ctx context.Context, cid string, rescheduleType clusters.RescheduleType, delayedUntil optional.Time) (operations.Operation, error) {
	return kf.operator.ModifyOnClusterWithoutRevChanging(ctx, cid, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			maintenanceTime, err := modifier.RescheduleMaintenance(ctx, cluster.ClusterID, rescheduleType, delayedUntil)
			if err != nil {
				return operations.Operation{}, err
			}

			op, err := kf.tasks.CreateFinishedTask(
				ctx,
				session,
				cluster.ClusterID,
				cluster.Revision,
				kfmodels.OperationTypeMaintenanceReschedule,
				kfmodels.MetadataRescheduleMaintenance{
					DelayedUntil: maintenanceTime,
				},
				true,
			)
			if err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
		clusterslogic.WithPermission(kfmodels.PermClustersUpdate),
	)
}
