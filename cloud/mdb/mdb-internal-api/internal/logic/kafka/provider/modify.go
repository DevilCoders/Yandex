package provider

import (
	"context"
	"sort"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/provider/internal/kfpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	modelsoptional "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/optional"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

type modifyClusterImplArgs struct {
	ClusterID          string
	Name               optional.String
	Description        optional.String
	SecurityGroupIDs   optional.Strings
	Labels             modelsoptional.Labels
	ConfigSpec         configSpecUpdate
	DeletionProtection optional.Bool
	MaintenanceWindow  modelsoptional.MaintenanceWindow
	IsPrestable        bool
}

type configSpecUpdate struct {
	Version         optional.String
	Kafka           kfmodels.KafkaConfigSpecUpdate
	ZooKeeper       kfmodels.ZookeeperConfigSpecUpdate
	ZoneID          optional.Strings
	BrokersCount    optional.Int64
	AssignPublicIP  optional.Bool
	UnmanagedTopics optional.Bool
	SchemaRegistry  optional.Bool
	Access          clusters.Access
}

func (kf *Kafka) ModifyMDBCluster(ctx context.Context, args kafka.ModifyMDBClusterArgs) (operations.Operation, error) {
	return kf.operator.ModifyOnCluster(ctx, args.ClusterID, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar, err := getPillarFromCluster(cluster)
			if err != nil {
				return operations.Operation{}, err
			}

			modifyClusterImplArgs := convertModifyMDBArgs(args, kf.IsEnvPrestable(cluster.Environment))

			return kf.modifyClusterImpl(ctx, session, reader, modifier, cluster, pillar, modifyClusterImplArgs)
		},
		clusterslogic.WithPermission(kfmodels.PermClustersUpdate),
	)
}

func (kf *Kafka) ModifyDataCloudCluster(ctx context.Context, args kafka.ModifyDataCloudClusterArgs) (operations.Operation, error) {
	return kf.operator.ModifyOnCluster(ctx, args.ClusterID, clusters.TypeKafka,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier,
			cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar, err := getPillarFromCluster(cluster)
			if err != nil {
				return operations.Operation{}, err
			}

			modifyClusterImplArgs, err := convertModifyDataCloudArgs(ctx, session, modifier, pillar, args, kf.IsEnvPrestable(cluster.Environment))
			if err != nil {
				return operations.Operation{}, err
			}

			return kf.modifyClusterImpl(ctx, session, reader, modifier, cluster, pillar, modifyClusterImplArgs)
		},
		clusterslogic.WithPermission(kfmodels.PermClustersUpdate),
	)
}

func convertModifyMDBArgs(args kafka.ModifyMDBClusterArgs, isPrestable bool) modifyClusterImplArgs {
	result := modifyClusterImplArgs{
		ClusterID:        args.ClusterID,
		Name:             args.Name,
		Description:      args.Description,
		SecurityGroupIDs: args.SecurityGroupIDs,
		Labels:           args.Labels,
		ConfigSpec: configSpecUpdate{
			Version:         args.ConfigSpec.Version,
			Kafka:           args.ConfigSpec.Kafka,
			ZooKeeper:       args.ConfigSpec.ZooKeeper,
			ZoneID:          args.ConfigSpec.ZoneID,
			BrokersCount:    args.ConfigSpec.BrokersCount,
			AssignPublicIP:  args.ConfigSpec.AssignPublicIP,
			UnmanagedTopics: args.ConfigSpec.UnmanagedTopics,
			SchemaRegistry:  args.ConfigSpec.SchemaRegistry,
			Access:          args.ConfigSpec.Access,
		},
		DeletionProtection: args.DeletionProtection,
		MaintenanceWindow:  args.MaintenanceWindow,
		IsPrestable:        isPrestable,
	}
	return result
}

func convertModifyDataCloudArgs(ctx context.Context, session sessions.Session, modifier clusterslogic.Modifier,
	pillar *kfpillars.Cluster, args kafka.ModifyDataCloudClusterArgs, isPrestable bool) (modifyClusterImplArgs, error) {
	kafkaZones, err := getUpdatedDataCloudKafkaZones(ctx, session, modifier, pillar, args.ConfigSpec.ZoneCount, args.ConfigSpec.BrokersCount)
	if err != nil {
		return modifyClusterImplArgs{}, err
	}

	result := modifyClusterImplArgs{
		ClusterID:   args.ClusterID,
		Name:        args.Name,
		Description: args.Description,
		Labels:      args.Labels,
		ConfigSpec: configSpecUpdate{
			Version:      args.ConfigSpec.Version,
			Kafka:        args.ConfigSpec.Kafka,
			ZoneID:       kafkaZones,
			BrokersCount: args.ConfigSpec.BrokersCount,
			Access:       args.ConfigSpec.Access,
		},
		IsPrestable: isPrestable,
	}
	return result, nil
}

func getUpdatedDataCloudKafkaZones(ctx context.Context, session sessions.Session, modifier clusterslogic.Modifier,
	pillar *kfpillars.Cluster, zoneCountOptional, brokersCountOptional optional.Int64) (optional.Strings, error) {
	if !zoneCountOptional.Valid {
		return optional.Strings{}, nil
	}

	zoneCount := zoneCountOptional.Int64
	if int(zoneCount) < len(pillar.Data.Kafka.ZoneID) {
		return optional.Strings{}, semerr.NotImplemented("removing zones is not implemented")
	} else if int(zoneCount) == len(pillar.Data.Kafka.ZoneID) {
		return optional.Strings{}, nil
	}

	var brokersCount int64
	if brokersCountOptional.Valid {
		brokersCount = brokersCountOptional.Int64
	} else {
		brokersCount = pillar.Data.Kafka.BrokersCount
	}

	oneHostMode := brokersCount == 1 && zoneCount == 1
	kafkaZones, _, err := selectDataCloudKafkaAndZookeeperHostZones(ctx, session, modifier, pillar.Data.CloudType,
		pillar.Data.RegionID, zoneCount, oneHostMode)
	if err != nil {
		return optional.Strings{}, err
	}

	newZones := subtractElements(kafkaZones, pillar.Data.Kafka.ZoneID)
	countToAddZones := len(kafkaZones) - len(pillar.Data.Kafka.ZoneID)

	return optional.Strings{
		Valid:   true,
		Strings: append(pillar.Data.Kafka.ZoneID, newZones[0:countToAddZones]...),
	}, nil
}

func subtractElements(array, elementsToSubtract []string) []string {
	mapElementsToSustract := make(map[string]bool)
	for _, el := range elementsToSubtract {
		mapElementsToSustract[el] = true
	}
	var results []string
	for _, el := range array {
		if _, ok := mapElementsToSustract[el]; !ok {
			results = append(results, el)
		}
	}

	return results
}

func (kf *Kafka) modifyClusterImpl(ctx context.Context, session sessions.Session, reader clusterslogic.Reader,
	modifier clusterslogic.Modifier, cluster clusterslogic.Cluster, pillar *kfpillars.Cluster,
	args modifyClusterImplArgs) (operations.Operation, error) {

	clusterChanges := common.GetClusterChanges()
	var changedSecurityGroups optional.Strings
	restart := false

	if args.SecurityGroupIDs.Valid && !slices.EqualAnyOrderStrings(args.SecurityGroupIDs.Strings, cluster.SecurityGroupIDs) {
		changedSecurityGroups.Set(slices.DedupStrings(args.SecurityGroupIDs.Strings))
		if err := kf.compute.ValidateSecurityGroups(ctx, changedSecurityGroups.Strings, cluster.NetworkID); err != nil {
			return operations.Operation{}, err
		}
		clusterChanges.HasChanges = true
	}

	if args.ConfigSpec.AssignPublicIP.Valid && args.ConfigSpec.AssignPublicIP.Bool != pillar.Data.Kafka.AssignPublicIP {
		pillar.Data.Kafka.AssignPublicIP = args.ConfigSpec.AssignPublicIP.Bool
		clusterChanges.HasChanges = true
		for fqdn := range pillar.Data.Kafka.Nodes {
			err := modifier.ModifyHostPublicIP(ctx, cluster.ClusterID, fqdn, cluster.Revision, pillar.Data.Kafka.AssignPublicIP)
			if err != nil {
				return operations.Operation{}, err
			}
		}
	}

	if args.ConfigSpec.UnmanagedTopics.Valid && args.ConfigSpec.UnmanagedTopics.Bool != pillar.Data.Kafka.UnmanagedTopics {
		if !args.ConfigSpec.UnmanagedTopics.Bool {
			return operations.Operation{}, semerr.NotImplemented("disabling topic_unmanaged is not implemented")
		}
		pillar.Data.Kafka.UnmanagedTopics = true
		clusterChanges.HasChanges = true
	}
	if args.ConfigSpec.SchemaRegistry.Valid && args.ConfigSpec.SchemaRegistry.Bool != pillar.Data.Kafka.SchemaRegistry {
		if !args.ConfigSpec.SchemaRegistry.Bool {
			return operations.Operation{}, semerr.NotImplemented("disabling schema_registry is not implemented")
		}
		pillar.Data.Kafka.SchemaRegistry = true
		clusterChanges.HasChanges = true
	}

	if args.ConfigSpec.Version.Valid && args.ConfigSpec.Version.String == "3.2" {
		return operations.Operation{}, semerr.NotImplemented("update to version 3.2 is not supported yet")
	}

	configHasChanges := optional.ApplyUpdate(&pillar.Data.Kafka.Config, &args.ConfigSpec.Kafka.Config)
	clusterChanges.HasChanges = clusterChanges.HasChanges || configHasChanges
	restart = restart || configHasChanges

	if args.ConfigSpec.Kafka.Resources.IsSet() {
		changes, extraTimeout, err := kf.modifyResources(ctx, reader, modifier, cluster.Cluster, hosts.RoleKafka, args.ConfigSpec.Kafka.Resources, session)
		if err != nil {
			return operations.Operation{}, err
		}

		clusterChanges.HasChanges = clusterChanges.HasChanges || changes
		clusterChanges.Timeout += extraTimeout
		optional.ApplyUpdate(&pillar.Data.Kafka.Resources, &args.ConfigSpec.Kafka.Resources)
	}

	hostsAdded, err := kf.modifyKafkaHosts(ctx, reader, modifier, session, cluster.Cluster, args, pillar)
	if err != nil {
		return operations.Operation{}, err
	}

	if args.ConfigSpec.ZooKeeper.Resources.IsSet() {
		if !pillar.HasZkSubcluster() {
			return operations.Operation{}, semerr.InvalidInput("cannot change zookeeper resources: " +
				"this cluster doesn't have dedicated zookeeper hosts")
		}

		changes, extraTimeout, err := kf.modifyResources(ctx, reader, modifier, cluster.Cluster, hosts.RoleZooKeeper, args.ConfigSpec.ZooKeeper.Resources, session)
		if err != nil {
			return operations.Operation{}, err
		}

		clusterChanges.HasChanges = clusterChanges.HasChanges || changes
		clusterChanges.Timeout += extraTimeout
		optional.ApplyUpdate(&pillar.Data.ZooKeeper.Resources, &args.ConfigSpec.ZooKeeper.Resources)
	}
	updatedAccess := args.ConfigSpec.Access.DataTransfer.Valid ||
		args.ConfigSpec.Access.Ipv4CidrBlocks != nil || args.ConfigSpec.Access.Ipv6CidrBlocks != nil
	if updatedAccess {
		if err := args.ConfigSpec.Access.ValidateAndSane(); err != nil {
			return operations.Operation{}, err
		}
		pillar.SetAccess(args.ConfigSpec.Access)
		clusterChanges.HasChanges = true
	}

	clusterChanges.HasMetadataChanges, err = modifier.ModifyClusterMetadata(ctx, cluster, args.Name, args.Labels)
	if err != nil {
		return operations.Operation{}, err
	}

	clusterChanges.HasMetaDBChanges, err = modifier.ModifyClusterMetadataParameters(ctx, cluster, args.Description, args.Labels, args.DeletionProtection, args.MaintenanceWindow)
	if err != nil {
		return operations.Operation{}, err
	}

	clusterChanges.HasChanges = clusterChanges.HasChanges || len(hostsAdded) > 0

	if args.ConfigSpec.Version.Valid && args.ConfigSpec.Version.String != pillar.Data.Kafka.Version {
		if !session.FeatureFlags.Has(kfmodels.KafkaAllowUpgradeFeatureFlag) {
			return operations.Operation{}, semerr.NotImplemented("changing version of the cluster is not implemented")
		}

		if clusterChanges.HasChanges {
			return operations.Operation{}, semerr.InvalidInput("Version update cannot be mixed with any other changes")
		}

		err := pillar.SetVersion(args.ConfigSpec.Version.String)
		if err != nil {
			return operations.Operation{}, err
		}

		err = modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar)
		if err != nil {
			return operations.Operation{}, xerrors.Errorf("failed to update pillar: %w", err)
		}

		return kf.tasks.UpgradeCluster(
			ctx,
			session,
			cluster.ClusterID,
			cluster.Revision,
			kfmodels.TaskTypeClusterUpgrade,
			kfmodels.OperationTypeClusterUpgrade,
			kfmodels.MetadataModifyCluster{},
		)
	}

	clusterChanges.TaskArgs = map[string]interface{}{
		"restart": restart,
	}
	if args.ConfigSpec.SchemaRegistry.Valid {
		clusterChanges.TaskArgs["run-highstate"] = true
	}
	if len(hostsAdded) > 0 {
		clusterChanges.TaskArgs["hosts_create"] = hostsAdded
	}
	if updatedAccess || len(hostsAdded) > 0 {
		clusterChanges.TaskArgs["update-firewall"] = true
	}

	if clusterChanges.HasChanges {
		err = modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar)
		if err != nil {
			return operations.Operation{}, xerrors.Errorf("failed to update pillar: %w", err)
		}
	}

	op, err := common.CreateClusterModifyOperation(kf.tasks, ctx, session, cluster.Cluster, searchAttributesExtractor, clusterChanges, changedSecurityGroups, getTaskCreationInfo())
	if err != nil {
		return operations.Operation{}, err
	}

	if clusterChanges.HasOnlyMetadbChanges() {
		if err := kf.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
			return operations.Operation{}, err
		}

		return op, nil
	}

	return op, nil
}

func (kf *Kafka) modifyKafkaHosts(ctx context.Context, reader clusterslogic.Reader, modifier clusterslogic.Modifier,
	session sessions.Session, cl clusters.Cluster, args modifyClusterImplArgs, pillar *kfpillars.Cluster) ([]string, error) {
	brokersCount := pillar.Data.Kafka.BrokersCount
	brokersCountBefore := pillar.Data.Kafka.BrokersCount
	brokersCountDiff := 0
	if args.ConfigSpec.BrokersCount.Valid {
		brokersCountDiff = int(args.ConfigSpec.BrokersCount.Int64 - pillar.Data.Kafka.BrokersCount)
		brokersCount = args.ConfigSpec.BrokersCount.Int64

		if brokersCountDiff < 0 {
			return nil, semerr.NotImplemented("decreasing number of brokers is not implemented")
		}
		if brokersCountDiff > 0 {
			if pillar.BrokersQuantity() == 1 {
				return nil, semerr.NotImplemented("increasing brokers count from one is not implemented")
			}
			if !pillar.HasZkSubcluster() {
				return nil, semerr.NotImplemented("changing list of zones on this cluster is not implemented")
			}
		}

		pillar.Data.Kafka.BrokersCount = brokersCount
	}

	zones := pillar.Data.Kafka.ZoneID
	var zonesAdded []string
	if args.ConfigSpec.ZoneID.Valid && unorderedListsDiffer(args.ConfigSpec.ZoneID.Strings, pillar.Data.Kafka.ZoneID) {
		// Check removed zones
		for _, zoneExisting := range pillar.Data.Kafka.ZoneID {
			zoneFound := false
			for _, zoneNew := range args.ConfigSpec.ZoneID.Strings {
				if zoneExisting == zoneNew {
					zoneFound = true
					break
				}
			}
			if !zoneFound {
				return nil, semerr.NotImplemented("removing zones is not implemented")
			}
		}

		if pillar.BrokersQuantity() == 1 {
			return nil, semerr.NotImplemented("increasing brokers count from one is not implemented")
		}
		if !pillar.HasZkSubcluster() {
			return nil, semerr.NotImplemented("changing list of zones on this cluster is not implemented")
		}
		for _, zoneNew := range args.ConfigSpec.ZoneID.Strings {
			if !pillar.HasZone(zoneNew) {
				zonesAdded = append(zonesAdded, zoneNew)
			}
		}
		pillar.Data.Kafka.ZoneID = args.ConfigSpec.ZoneID.Strings
	}

	if len(zonesAdded) == 0 && brokersCountDiff == 0 {
		return nil, nil
	}

	currentHosts, _, _, err := reader.ListHosts(ctx, cl.ClusterID, 0, 0)
	if err != nil {
		return nil, err
	}
	subnetsID := make([]string, 0)
	for _, host := range clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleZooKeeper) {
		subnetsID = append(subnetsID, host.SubnetID)
	}

	kafkaHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleKafka)
	if len(kafkaHosts) == 0 {
		return nil, xerrors.New("modifying kafka cluster without kafka brokers")
	}
	anyKafkaHost := kafkaHosts[0]

	_, subnets, err := kf.findSubnets(ctx, cl.NetworkID, subnetsID)
	if err != nil {
		return nil, xerrors.Errorf("failed to find subnets: %w", err)
	}

	kafkaResources := pillar.Data.Kafka.Resources

	kafkaHostGroup := clusterslogic.HostGroup{
		Role:                   hosts.RoleKafka,
		NewResourcePresetExtID: optional.NewString(kafkaResources.ResourcePresetExtID),
		DiskTypeExtID:          kafkaResources.DiskTypeExtID,
		NewDiskSize:            optional.NewInt64(kafkaResources.DiskSize),
		HostsCurrent:           make([]clusterslogic.ZoneHosts, 0),
		HostsToAdd:             make([]clusterslogic.ZoneHosts, 0),
	}
	for _, zoneID := range zones {
		kafkaHostGroup.HostsCurrent = append(kafkaHostGroup.HostsCurrent, clusterslogic.ZoneHosts{ZoneID: zoneID, Count: brokersCountBefore})
	}
	for _, zoneID := range zones {
		kafkaHostGroup.HostsToAdd = append(kafkaHostGroup.HostsToAdd, clusterslogic.ZoneHosts{ZoneID: zoneID, Count: int64(brokersCountDiff)})
	}
	for _, zoneID := range zonesAdded {
		kafkaHostGroup.HostsToAdd = append(kafkaHostGroup.HostsToAdd, clusterslogic.ZoneHosts{ZoneID: zoneID, Count: brokersCount})
	}

	resolvedHostGroups, _, err := modifier.ValidateResources(
		ctx,
		session,
		clusters.TypeKafka,
		kafkaHostGroup,
	)
	if err != nil {
		return nil, err
	}
	kafkaResolvedHostGroup := resolvedHostGroups.Single()

	var addHostsArgs []models.AddHostArgs
	kafkaHostArgs := models.AddHostArgs{
		SubClusterID:   anyKafkaHost.SubClusterID,
		SpaceLimit:     kafkaResources.DiskSize,
		DiskTypeExtID:  kafkaResources.DiskTypeExtID,
		Revision:       cl.Revision,
		ClusterID:      cl.ClusterID,
		AssignPublicIP: pillar.Data.Kafka.AssignPublicIP,
	}

	addHostsArgs, err = kf.addKafkaHosts(ctx, session, modifier, addHostsArgs, pillar.Data.CloudType, subnets,
		kafkaResolvedHostGroup.TargetResourcePreset(), kafkaHostArgs, pillar, kafkaHostGroup.HostsToAdd, kafkaHostGroup.HostsCurrent)
	if err != nil {
		return nil, xerrors.Errorf("failed to add broker hosts: %w", err)
	}
	_, err = modifier.AddHosts(ctx, addHostsArgs)
	if err != nil {
		return nil, xerrors.Errorf("failed to add hosts: %w", err)
	}

	hostsAdded := make([]string, 0, len(addHostsArgs))
	for _, addHost := range addHostsArgs {
		hostsAdded = append(hostsAdded, addHost.FQDN)
	}
	return hostsAdded, err
}

func (kf *Kafka) modifyResources(ctx context.Context, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusters.Cluster, role hosts.Role, targetResources models.ClusterResourcesSpec, session sessions.Session) (bool, time.Duration, error) {
	currentHosts, _, _, err := reader.ListHosts(ctx, cluster.ClusterID, 0, 0)
	if err != nil {
		return false, 0, err
	}

	hostsWithRole := clusterslogic.GetHostsWithRole(currentHosts, role)
	if len(hostsWithRole) == 0 {
		return false, 0, xerrors.Errorf("failed to find host with role %q within cluster %q", role.String(), cluster.ClusterID)
	}
	anyHost := hostsWithRole[0]

	if targetResources.DiskTypeExtID.Valid && targetResources.DiskTypeExtID.String != anyHost.DiskTypeExtID {
		return false, 0, semerr.InvalidInput("type of disk cannot be changed")
	}

	zoneHostsByZone := make(map[string]*clusterslogic.ZoneHosts)
	for _, host := range hostsWithRole {
		zoneHosts, ok := zoneHostsByZone[host.ZoneID]
		if !ok {
			zoneHosts = &clusterslogic.ZoneHosts{ZoneID: host.ZoneID}
			zoneHostsByZone[host.ZoneID] = zoneHosts
		}
		zoneHosts.Count += 1
	}
	zoneHostsList := make([]clusterslogic.ZoneHosts, 0, len(zoneHostsByZone))
	for _, zoneHosts := range zoneHostsByZone {
		zoneHostsList = append(zoneHostsList, *zoneHosts)
	}
	sort.Slice(zoneHostsList, func(i, j int) bool {
		return zoneHostsList[i].ZoneID < zoneHostsList[j].ZoneID
	})

	hostGroup := clusterslogic.HostGroup{
		Role:                       role,
		CurrentResourcePresetExtID: optional.NewString(anyHost.ResourcePresetExtID),
		NewResourcePresetExtID:     targetResources.ResourcePresetExtID,
		DiskTypeExtID:              anyHost.DiskTypeExtID,
		CurrentDiskSize:            optional.NewInt64(anyHost.SpaceLimit),
		NewDiskSize:                targetResources.DiskSize,
		HostsCurrent:               zoneHostsList,
		SkipValidations: clusterslogic.SkipValidations{
			DecommissionedZone: role == hosts.RoleZooKeeper,
		},
	}

	resolvedHostGroups, hasChanges, err := modifier.ValidateResources(
		ctx,
		session,
		clusters.TypeKafka,
		hostGroup,
	)
	if err != nil {
		return false, 0, err
	}

	if !hasChanges {
		return false, 0, nil
	}

	resolvedHostGroup := resolvedHostGroups.Single()

	var extraTimeout time.Duration
	for _, host := range hostsWithRole {
		args := models.ModifyHostArgs{
			ClusterID:        host.ClusterID,
			FQDN:             host.FQDN,
			Revision:         cluster.Revision,
			SpaceLimit:       resolvedHostGroup.TargetDiskSize(),
			ResourcePresetID: resolvedHostGroup.TargetResourcePreset().ID,
			DiskTypeExtID:    resolvedHostGroup.DiskTypeExtID,
		}

		err = modifier.ModifyHost(ctx, args)
		if err != nil {
			return false, 0, err
		}

		// expect 1 GB to be transferred in 60 seconds
		extraTimeout += time.Duration(host.SpaceLimit*60/(1<<30)) * time.Second
	}

	return true, extraTimeout, nil
}

func (kf *Kafka) toSearchQueue(ctx context.Context, folderCoords metadb.FolderCoords, op operations.Operation) error {
	return kf.search.StoreDoc(
		ctx,
		kafkaService,
		folderCoords.FolderExtID,
		folderCoords.CloudExtID,
		op,
		searchAttributesExtractor,
	)
}

func unorderedListsDiffer(a, b []string) bool {
	// If one is nil, the other must also be nil.
	if (a == nil) != (b == nil) {
		return true
	}

	if len(a) != len(b) {
		return true
	}

	sort.Strings(a)
	sort.Strings(b)

	for i := range a {
		if a[i] != b[i] {
			return true
		}
	}

	return false
}

func getTaskCreationInfo() common.TaskCreationInfo {
	return common.TaskCreationInfo{
		ClusterModifyTask:      kfmodels.TaskTypeClusterModify,
		ClusterModifyOperation: kfmodels.OperationTypeClusterModify,

		MetadataUpdateTask:      kfmodels.TaskTypeMetadataUpdate,
		MetadataUpdateOperation: kfmodels.OperationTypeMetadataUpdate,

		SearchService: kafkaService,
	}
}
