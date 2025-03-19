package provider

import (
	"context"
	"sort"

	"a.yandex-team.ru/cloud/mdb/internal/compute"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	taskslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch/osmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch/provider/internal/ospillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

const (
	dataNodeSubClusterName   = "datanode_subcluster"
	masterNodeSubClusterName = "masternode_subcluster"
	passwordGenLen           = 16
	openSearchService        = "managed-opensearch"
	dashboardsEncKeyLen      = 32
)

func (es *OpenSearch) Cluster(ctx context.Context, cid string) (osmodels.Cluster, error) {
	var cl osmodels.Cluster
	if err := es.operator.ReadCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			clExt, err := reader.ClusterExtendedByClusterID(ctx, cid, clusters.TypeOpenSearch, models.VisibilityVisible, session)
			if err != nil {
				return err
			}
			cl, err = es.toOpenSearchModel(ctx, reader, clExt)
			return err
		},
		clusterslogic.WithPermission(osmodels.PermClustersGet),
	); err != nil {
		return osmodels.Cluster{}, err
	}

	return cl, nil
}

func (es *OpenSearch) Clusters(ctx context.Context, folderExtID string, limit, offset int64) ([]osmodels.Cluster, error) {
	var cls []osmodels.Cluster
	if err := es.operator.ReadOnFolder(ctx, folderExtID,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			var err error
			clsExt, err := reader.ClustersExtended(ctx, models.ListClusterArgs{
				ClusterType: clusters.TypeOpenSearch,
				FolderID:    session.FolderCoords.FolderID,
				Limit:       optional.NewInt64(limit),
				Offset:      offset,
				Visibility:  models.VisibilityVisible,
			}, session)
			if err != nil {
				return err
			}

			for _, clExt := range clsExt {
				cl, err := es.toOpenSearchModel(ctx, reader, clExt)
				if err != nil {
					return err
				}
				cls = append(cls, cl)
			}
			return nil
		},
		clusterslogic.WithPermission(osmodels.PermClustersGet),
	); err != nil {
		return nil, err
	}

	return cls, nil
}

func (es *OpenSearch) CreateCluster(ctx context.Context, args opensearch.CreateClusterArgs) (operations.Operation, error) {
	return es.operator.Create(ctx, args.FolderExtID,
		func(ctx context.Context, session sessions.Session, creator clusterslogic.Creator) (clusters.Cluster, operations.Operation, error) {
			if err := models.ClusterNameValidator.ValidateString(args.Name); err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			if !args.ConfigSpec.AdminPassword.Valid {
				return clusters.Cluster{}, operations.Operation{}, semerr.FailedPrecondition("admin password must be specified")
			}

			if args.ConfigSpec.AdminPassword.Valid {
				if err := osmodels.ValidateAdminPassword(args.ConfigSpec.AdminPassword.Value); err != nil {
					return clusters.Cluster{}, operations.Operation{}, err
				}
			}

			// validtate extensions uri
			for i := range args.ExtensionSpecs {
				if err := es.ValidateExtensionURI(args.ExtensionSpecs[i].Name, args.ExtensionSpecs[i].URI); err != nil {
					return clusters.Cluster{}, operations.Operation{}, err
				}
			}

			clusterHosts, err := splitHosts(args.HostSpec)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}
			if len(clusterHosts.DataNodes) < 1 {
				return clusters.Cluster{}, operations.Operation{}, semerr.FailedPrecondition("at least 1 data node needed")
			}

			if len(clusterHosts.MasterNodes) == 0 && len(clusterHosts.DataNodes) == 2 {
				return clusters.Cluster{}, operations.Operation{}, semerr.FailedPrecondition("at least 3 node required for cluster resiliency")
			}

			config, err := args.ConfigSpec.Config.Get()
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}
			err = config.DataNode.Validate(true)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, semerr.InvalidInputf("invalid data node config: %s", err)
			}

			if len(clusterHosts.MasterNodes) > 0 {
				if !config.MasterNode.Valid {
					return clusters.Cluster{}, operations.Operation{}, semerr.InvalidInput("master nodes specified but no resources provided")
				}
				err = config.MasterNode.Value.Validate(true)
				if err != nil {
					return clusters.Cluster{}, operations.Operation{}, semerr.InvalidInputf("invalid master node config: %s", err)
				}
			}

			hostGroups := make([]clusterslogic.HostGroup, 0, 2)
			// DataNode
			{
				res := config.DataNode.Resources.MustOptionals()
				hg := clusterslogic.HostGroup{
					Role:                   hosts.RoleOpenSearchDataNode,
					NewResourcePresetExtID: optional.NewString(res.ResourcePresetExtID),
					DiskTypeExtID:          res.DiskTypeExtID,
					NewDiskSize:            optional.NewInt64(res.DiskSize),
					HostsToAdd:             make([]clusterslogic.ZoneHosts, 0, len(clusterHosts.DataNodes)),
					SkipValidations: clusterslogic.SkipValidations{
						MaxHosts: session.FeatureFlags.Has(osmodels.OpenSearchAllowUnlimitedHostsFeatureFlag),
					},
				}
				for _, node := range clusterHosts.DataNodes {
					hg.HostsToAdd = append(hg.HostsToAdd, clusterslogic.ZoneHosts{ZoneID: node.ZoneID, Count: 1})
				}

				hostGroups = append(hostGroups, hg)
			}
			// MasterNode
			if len(clusterHosts.MasterNodes) > 0 {
				res := config.MasterNode.Value.Resources.MustOptionals()
				hg := clusterslogic.HostGroup{
					Role:                   hosts.RoleOpenSearchMasterNode,
					NewResourcePresetExtID: optional.NewString(res.ResourcePresetExtID),
					DiskTypeExtID:          res.DiskTypeExtID,
					NewDiskSize:            optional.NewInt64(res.DiskSize),
					HostsToAdd:             make([]clusterslogic.ZoneHosts, 0, len(clusterHosts.MasterNodes)),
				}
				for _, node := range clusterHosts.MasterNodes {
					hg.HostsToAdd = append(hg.HostsToAdd, clusterslogic.ZoneHosts{ZoneID: node.ZoneID, Count: 1})
				}

				hostGroups = append(hostGroups, hg)
			}

			resolvedHostGroups, _, err := creator.ValidateResources(
				ctx,
				session,
				clusters.TypeOpenSearch,
				hostGroups...,
			)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			if args.ServiceAccountID != "" {
				resourcePreset := resolvedHostGroups.MustByHostRole(hosts.RoleOpenSearchDataNode).TargetResourcePreset()
				if err := es.validateServiceAccountAccess(ctx, session, args.ServiceAccountID, resourcePreset.VType); err != nil {
					return clusters.Cluster{}, operations.Operation{}, err

				}
			}

			network, subnets, err := es.compute.NetworkAndSubnets(ctx, args.NetworkID)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			if len(args.SecurityGroupIDs) > 0 {
				args.SecurityGroupIDs = slices.DedupStrings(args.SecurityGroupIDs)
				// validate security groups after NetworkID validation, cause we validate SG above network
				if err := es.compute.ValidateSecurityGroups(ctx, args.SecurityGroupIDs, args.NetworkID); err != nil {
					return clusters.Cluster{}, operations.Operation{}, err
				}
			}

			cluster, clusterPillar, err := es.createCluster(ctx, creator, args, session.FolderCoords.FolderID, network)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			// Create data subcluster pillar
			dataPillar := ospillars.NewDataNodeSubCluster()
			dataPillar.Data.OpenSearch.Config.DataNode = config.DataNode.Config
			if len(clusterHosts.MasterNodes) == 0 {
				dataPillar.Data.OpenSearch.IsMaster = true
			}

			dataNodeResources := config.DataNode.Resources.MustOptionals()
			err = es.createDataNodeSubcluster(
				ctx,
				session,
				creator,
				cluster,
				clusterHosts.DataNodes,
				subnets,
				resolvedHostGroups.MustByHostRole(hosts.RoleOpenSearchDataNode).TargetResourcePreset(),
				dataNodeResources.DiskTypeExtID,
				dataNodeResources.DiskSize,
				dataPillar)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			if len(clusterHosts.MasterNodes) > 0 {
				masterNodeResources := config.MasterNode.Value.Resources.MustOptionals()
				err = es.createMasterNodeSubcluster(
					ctx,
					session,
					creator,
					cluster,
					clusterHosts.MasterNodes,
					subnets,
					resolvedHostGroups.MustByHostRole(hosts.RoleOpenSearchMasterNode).TargetResourcePreset(),
					masterNodeResources.DiskTypeExtID,
					masterNodeResources.DiskSize)
				if err != nil {
					return clusters.Cluster{}, operations.Operation{}, err
				}
			}

			var extensions []string
			for _, e := range clusterPillar.Extensions().GetList() {
				extensions = append(extensions, e.ID)
			}

			if args.RestoreFrom == "" {
				op, err := es.tasks.CreateCluster(
					ctx,
					session,
					cluster.ClusterID,
					cluster.Revision,
					osmodels.TaskTypeClusterCreate,
					osmodels.OperationTypeClusterCreate,
					osmodels.MetadataCreateCluster{},
					optional.String{}, // We pass additional buckets via options
					args.SecurityGroupIDs,
					openSearchService,
					searchAttributesExtractor,
					taskslogic.CreateTaskArgs(map[string]interface{}{
						"service_account_id": clusterPillar.Data.ServiceAccountID,
						"major_version":      clusterPillar.Version().EncodedID(),
						"s3_buckets":         clusterPillar.Data.S3Buckets,
						"extensions":         extensions,
					}),
				)
				if err != nil {
					return clusters.Cluster{}, operations.Operation{}, err
				}

				return cluster, op, nil
			}

			cid, backupID, err := backups.DecodeGlobalBackupID(args.RestoreFrom)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			// restore task
			op, err := es.tasks.CreateCluster(
				ctx,
				session,
				cluster.ClusterID,
				cluster.Revision,
				osmodels.TaskTypeClusterRestore,
				osmodels.OperationTypeClusterRestore,
				osmodels.MetadataRestoreCluster{BackupID: args.RestoreFrom},
				optional.String{}, // We pass additional buckets via options
				args.SecurityGroupIDs,
				openSearchService,
				searchAttributesExtractor,
				taskslogic.CreateTaskArgs(map[string]interface{}{
					"restore_from": map[string]interface{}{
						"bucket": es.cfg.S3BucketName(cid),
						"backup": backupID,
					},
					"service_account_id": clusterPillar.Data.ServiceAccountID,
					"major_version":      clusterPillar.Version().EncodedID(),
					"s3_buckets":         clusterPillar.Data.S3Buckets,
					"extensions":         extensions,
				}),
			)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			return cluster, op, nil
		},
		clusterslogic.WithPermission(osmodels.PermClustersCreate),
	)
}

func (es *OpenSearch) DeleteCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return es.operator.Delete(ctx, cid, clusters.TypeOpenSearch,
		func(ctx context.Context, session sessions.Session, cluster clusterslogic.Cluster, _ clusterslogic.Reader) (operations.Operation, error) {
			var pillar ospillars.Cluster
			err := cluster.Pillar(&pillar)
			if err != nil {
				return operations.Operation{}, err
			}

			var s3Buckets taskslogic.DeleteClusterS3Buckets

			// old clusters may not have s3 buckets
			if len(pillar.Data.S3Bucket) > 0 {
				s3Buckets = taskslogic.DeleteClusterS3Buckets{
					"backup": pillar.Data.S3Bucket,
				}
			}

			if len(pillar.Data.S3Buckets) > 0 {
				s3Buckets = taskslogic.DeleteClusterS3Buckets(pillar.Data.S3Buckets)
			}

			op, err := es.tasks.DeleteCluster(
				ctx,
				session,
				cluster.ClusterID,
				cluster.Revision,
				taskslogic.DeleteClusterTaskTypes{
					Delete:   osmodels.TaskTypeClusterDelete,
					Metadata: osmodels.TaskTypeClusterDeleteMetadata,
					Purge:    osmodels.TaskTypeClusterPurge,
				},
				osmodels.OperationTypeClusterDelete,
				s3Buckets,
			)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := es.search.StoreDocDelete(ctx, openSearchService, session.FolderCoords.FolderExtID, session.FolderCoords.CloudExtID, op); err != nil {
				return operations.Operation{}, err
			}
			return op, nil
		},
		clusterslogic.WithPermission(osmodels.PermClustersDelete),
	)
}

func (es *OpenSearch) StartCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeOpenSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, _ clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			op, err := es.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      osmodels.TaskTypeClusterStart,
					OperationType: osmodels.OperationTypeClusterStart,
					Revision:      cluster.Revision,
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}
			return op, nil
		},
		clusterslogic.WithPermission(osmodels.PermClustersStart),
	)
}

func (es *OpenSearch) StopCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeOpenSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, _ clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			op, err := es.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      osmodels.TaskTypeClusterStop,
					OperationType: osmodels.OperationTypeClusterStop,
					Revision:      cluster.Revision,
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}
			return op, nil
		},
		clusterslogic.WithPermission(osmodels.PermClustersStop),
	)
}

func (es *OpenSearch) ListHosts(ctx context.Context, cid string, limit, offset int64) ([]hosts.HostExtended, pagination.OffsetPageToken, error) {
	res, newOffset, hasMore, err := clusterslogic.ListHosts(ctx, es.operator, cid, clusters.TypeOpenSearch, limit, offset, clusterslogic.WithPermission(osmodels.PermClustersGet))
	if err != nil {
		return nil, pagination.OffsetPageToken{}, err
	}

	return res, pagination.OffsetPageToken{
		Offset: newOffset,
		More:   hasMore,
	}, nil
}

func (es *OpenSearch) AddHosts(ctx context.Context, cid string, hsts []osmodels.Host) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeOpenSearch,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {

			if len(hsts) > 1 {
				return operations.Operation{}, semerr.NotImplemented("adding multiple hosts at once is not supported yet")
			}

			if len(hsts) < 1 {
				return operations.Operation{}, semerr.FailedPrecondition("no hosts to add are specified")
			}

			clusterPillar := ospillars.NewCluster()
			if err := cluster.Pillar(clusterPillar); err != nil {
				return operations.Operation{}, err
			}

			hostToAdd := hsts[0]
			if hostToAdd.Role != hosts.RoleOpenSearchDataNode {
				return operations.Operation{}, semerr.NotImplemented("only data nodes can be added")
			}

			currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cluster.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			dataHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleOpenSearchDataNode)
			if len(dataHosts) == 0 {
				return operations.Operation{}, xerrors.New("adding hosts to cluster without data nodes")
			}
			anyDataHost := dataHosts[0]

			hostGroup := clusterslogic.HostGroup{
				Role:                       hosts.RoleOpenSearchDataNode,
				CurrentResourcePresetExtID: optional.NewString(anyDataHost.ResourcePresetExtID),
				DiskTypeExtID:              anyDataHost.DiskTypeExtID,
				CurrentDiskSize:            optional.NewInt64(anyDataHost.SpaceLimit),
				HostsCurrent:               clusterslogic.ZoneHostsListFromHosts(dataHosts),
				HostsToAdd: []clusterslogic.ZoneHosts{
					{
						ZoneID: hostToAdd.ZoneID,
						Count:  1,
					},
				},
				SkipValidations: clusterslogic.SkipValidations{
					MaxHosts: session.FeatureFlags.Has(osmodels.OpenSearchAllowUnlimitedHostsFeatureFlag),
				},
			}

			resolvedHostGroups, _, err := modifier.ValidateResources(
				ctx,
				session,
				clusters.TypeOpenSearch,
				hostGroup,
			)
			if err != nil {
				return operations.Operation{}, err
			}
			resolvedHostGroup := resolvedHostGroups.Single()

			_, subnets, err := es.compute.NetworkAndSubnets(ctx, cluster.NetworkID)
			if err != nil {
				return operations.Operation{}, err
			}

			createdHosts, err := es.createHosts(
				ctx,
				session,
				modifier,
				cid,
				anyDataHost.SubClusterID,
				[]osmodels.Host{hostToAdd},
				subnets,
				resolvedHostGroup.TargetResourcePreset(),
				resolvedHostGroup.DiskTypeExtID,
				resolvedHostGroup.TargetDiskSize(),
				cluster.Revision,
			)
			if err != nil {
				return operations.Operation{}, err
			}

			op, err := es.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      osmodels.TaskTypeHostCreate,
					OperationType: osmodels.OperationTypeHostCreate,
					Revision:      cluster.Revision,
					Metadata: osmodels.MetadataCreateHost{
						HostNames: []string{createdHosts[0].FQDN},
					},
					TaskArgs: map[string]interface{}{
						"host":               createdHosts[0].FQDN,
						"subcid":             createdHosts[0].SubClusterID,
						"service_account_id": clusterPillar.Data.ServiceAccountID,
						"major_version":      clusterPillar.Version().EncodedID(),
					},
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}
			return op, nil
		},
		clusterslogic.WithPermission(osmodels.PermClustersUpdate),
	)
}

func (es *OpenSearch) DeleteHosts(ctx context.Context, cid string, fqdns []string) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeOpenSearch,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			if len(fqdns) > 1 {
				return operations.Operation{}, semerr.NotImplemented("deleting multiple hosts at once is not supported yet")
			}

			if len(fqdns) < 1 {
				return operations.Operation{}, semerr.FailedPrecondition("no hosts to delete are specified")
			}

			currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cluster.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			hostToDelete, err := clusterslogic.HostExtendedByFQDN(currentHosts, fqdns[0])
			if err != nil {
				return operations.Operation{}, err
			}

			dataHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleOpenSearchDataNode)
			if len(dataHosts) == 1 {
				return operations.Operation{}, semerr.FailedPrecondition("the only data node cannot be removed")
			}
			if len(dataHosts) < 3 {
				return operations.Operation{}, semerr.FailedPrecondition("there must be at least 2 data nodes left")
			}
			anyDataHost := dataHosts[0]

			hostGroup := clusterslogic.HostGroup{
				Role:                       hosts.RoleOpenSearchDataNode,
				CurrentResourcePresetExtID: optional.NewString(anyDataHost.ResourcePresetExtID),
				DiskTypeExtID:              anyDataHost.DiskTypeExtID,
				CurrentDiskSize:            optional.NewInt64(anyDataHost.SpaceLimit),
				HostsCurrent:               clusterslogic.ZoneHostsListFromHosts(dataHosts),
				HostsToRemove: []clusterslogic.ZoneHosts{
					{
						ZoneID: hostToDelete.ZoneID,
						Count:  1,
					},
				},
				SkipValidations: clusterslogic.SkipValidations{
					MaxHosts: true,
				},
			}

			_, _, err = modifier.ValidateResources(
				ctx,
				session,
				clusters.TypeOpenSearch,
				hostGroup,
			)
			if err != nil {
				return operations.Operation{}, err
			}

			deletedHosts, err := modifier.DeleteHosts(ctx, cid, []string{hostToDelete.FQDN}, cluster.Revision)
			if err != nil {
				return operations.Operation{}, err

			}

			op, err := es.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      osmodels.TaskTypeHostDelete,
					OperationType: osmodels.OperationTypeHostDelete,
					Revision:      cluster.Revision,
					Metadata: osmodels.MetadataDeleteHost{
						HostNames: []string{deletedHosts[0].FQDN},
					},
					TaskArgs: map[string]interface{}{
						"host": map[string]interface{}{
							"fqdn":     deletedHosts[0].FQDN,
							"subcid":   deletedHosts[0].SubClusterID,
							"vtype":    deletedHosts[0].VType,
							"vtype_id": deletedHosts[0].VTypeID.String,
						},
					},
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}
			return op, nil
		},
		clusterslogic.WithPermission(osmodels.PermClustersUpdate),
	)
}

func searchAttributesExtractor(cluster clusterslogic.Cluster) (map[string]interface{}, error) {
	pillar := ospillars.NewCluster()
	if err := cluster.Pillar(pillar); err != nil {
		return nil, err
	}

	searchAttributes := make(map[string]interface{})

	users := make([]string, 0, len(pillar.Data.OpenSearch.Users))
	for name, props := range pillar.Data.OpenSearch.Users {
		if props.Internal {
			continue
		}
		users = append(users, name)
	}
	sort.Strings(users)
	searchAttributes["users"] = users

	return searchAttributes, nil
}

func (es *OpenSearch) toSearchQueue(ctx context.Context, folderCoords metadb.FolderCoords, op operations.Operation) error {
	return es.search.StoreDoc(
		ctx,
		openSearchService,
		folderCoords.FolderExtID,
		folderCoords.CloudExtID,
		op,
		searchAttributesExtractor,
	)
}

func (es *OpenSearch) createSystemUser(name string) (ospillars.UserData, error) {
	passwordEnc, err := crypto.GenerateEncryptedPassword(es.cryptoProvider, passwordGenLen, nil)
	if err != nil {
		return ospillars.UserData{}, err
	}
	return ospillars.UserData{
		Name:     name,
		Password: passwordEnc,
		Internal: true,
	}, nil
}

func (es *OpenSearch) createClusterPillar(args opensearch.CreateClusterArgs, clusterID string, privateKey []byte) (*ospillars.Cluster, error) {
	// Create cluster pillar
	pillar := ospillars.NewCluster()

	defaultVersion, err := es.supportedVersions.DefaultVersion()
	if err != nil {
		return nil, err
	}
	pillar.Data.OpenSearch.Version = args.ConfigSpec.Version.GetDefault(defaultVersion)

	pillar.Data.ServiceAccountID = args.ServiceAccountID
	pillar.Data.OpenSearch.AutoBackups.Enabled = es.cfg.Elasticsearch.EnableAutoBackups

	// cluster plugins
	if cfg, err := args.ConfigSpec.Config.Get(); err == nil {
		err := pillar.SetPlugins(cfg.Plugins...)
		if err != nil {
			return nil, err
		}
	}

	//
	privateKeyEnc, err := es.cryptoProvider.Encrypt(privateKey)
	if err != nil {
		return nil, err
	}
	pillar.Data.ClusterPrivateKey = privateKeyEnc

	s3Buckets := make(map[string]string)
	if es.cfg.Elasticsearch.EnableAutoBackups {
		s3Buckets["secure_backups"] = es.cfg.S3BucketName(clusterID)
	} else {
		// plain old bucket for porto now
		s3Buckets["backup"] = es.cfg.S3BucketName(clusterID)
	}
	pillar.Data.S3Buckets = s3Buckets

	if es.cfg.E2E.IsClusterE2E(args.Name, args.FolderExtID) {
		pillar.Data.MDBMetrics = pillars.NewDisabledMDBMetrics()
		pillar.Data.Billing = pillars.NewDisabledBilling()
		pillar.Data.UseYASMAgent = new(bool)
		pillar.Data.SuppressExternalYASMAgent = true
		pillar.Data.ShipLogs = new(bool)
		pillar.Data.MDBHealth = pillars.NewMDBHealthWithDisabledAggregate()
	}

	for _, sysUser := range osmodels.SystemUsers {
		userdata, err := es.createSystemUser(sysUser)
		if err != nil {
			return nil, err
		}
		err = pillar.AddUser(userdata)
		if err != nil {
			return nil, err
		}
	}

	if args.ConfigSpec.AdminPassword.Valid {
		adminPasswordEnc, err := es.cryptoProvider.Encrypt([]byte(args.ConfigSpec.AdminPassword.Value.Unmask()))
		if err != nil {
			return nil, err
		}
		err = pillar.AddUser(ospillars.UserData{
			Name:     osmodels.UserAdmin,
			Password: adminPasswordEnc,
			Internal: false,
		})
		if err != nil {
			return nil, xerrors.Errorf("failed to add user %q operation: %w", osmodels.UserAdmin, err)
		}
	}

	exts := pillar.Extensions()
	for _, s := range args.ExtensionSpecs {
		id, err := es.extIDGen.Generate()
		if err != nil {
			return nil, err
		}
		if err := exts.Add(id, s.Name, s.URI, s.Disabled); err != nil {
			return nil, err
		}
	}
	pillar.SetExtensions(exts)

	pillar.SetAccess(args.ConfigSpec.Access)

	return pillar, nil
}

func (es *OpenSearch) createCluster(ctx context.Context, creator clusterslogic.Creator, args opensearch.CreateClusterArgs, folderID int64, network networkProvider.Network) (clusters.Cluster, *ospillars.Cluster, error) {
	cluster, privateKey, err := creator.CreateCluster(ctx, models.CreateClusterArgs{
		Name:               args.Name,
		ClusterType:        clusters.TypeOpenSearch,
		Environment:        args.Environment,
		NetworkID:          network.ID,
		FolderID:           folderID,
		Description:        args.Description,
		Labels:             args.Labels,
		DeletionProtection: args.DeletionProtection,
		MaintenanceWindow:  args.MaintenanceWindow,
	})
	if err != nil {
		return clusters.Cluster{}, nil, err
	}

	clusterPillar, err := es.createClusterPillar(args, cluster.ClusterID, privateKey)
	if err != nil {
		return clusters.Cluster{}, nil, err
	}
	err = creator.AddClusterPillar(ctx, cluster.ClusterID, cluster.Revision, clusterPillar)
	if err != nil {
		return clusters.Cluster{}, nil, err
	}

	return cluster, clusterPillar, nil
}

func (es *OpenSearch) createDataNodeSubcluster(
	ctx context.Context,
	session sessions.Session,
	creator clusterslogic.Creator,
	cluster clusters.Cluster,
	hsts []osmodels.Host,
	subnets []networkProvider.Subnet, resourcePreset resources.Preset, diskTypeExtID string, diskSize int64, pillar *ospillars.DataNodeSubCluster) error {
	dataSubCluster, err := creator.CreateSubCluster(ctx, models.CreateSubClusterArgs{
		ClusterID: cluster.ClusterID,
		Name:      dataNodeSubClusterName,
		Roles:     []hosts.Role{hosts.RoleOpenSearchDataNode},
		Revision:  cluster.Revision,
	})
	if err != nil {
		return err
	}

	err = creator.AddSubClusterPillar(ctx, dataSubCluster.ClusterID, dataSubCluster.SubClusterID, cluster.Revision, pillar)
	if err != nil {
		return xerrors.Errorf("failed to update pillar: %w", err)
	}

	_, err = es.createHosts(ctx, session, creator, cluster.ClusterID, dataSubCluster.SubClusterID, hsts, subnets, resourcePreset, diskTypeExtID, diskSize, cluster.Revision)
	return err
}

func (es *OpenSearch) createMasterNodeSubcluster(
	ctx context.Context,
	session sessions.Session,
	creator clusterslogic.Creator,
	cluster clusters.Cluster,
	hsts []osmodels.Host,
	subnets []networkProvider.Subnet, resourcePreset resources.Preset, diskTypeExtID string, diskSize int64) error {
	masterSubCluster, err := creator.CreateSubCluster(ctx, models.CreateSubClusterArgs{
		ClusterID: cluster.ClusterID,
		Name:      masterNodeSubClusterName,
		Roles:     []hosts.Role{hosts.RoleOpenSearchMasterNode},
		Revision:  cluster.Revision,
	})
	if err != nil {
		return err
	}

	// Create subcluster pillar
	pillar := ospillars.NewMasterNodeSubCluster()
	err = creator.AddSubClusterPillar(ctx, masterSubCluster.ClusterID, masterSubCluster.SubClusterID, cluster.Revision, pillar)
	if err != nil {
		return xerrors.Errorf("failed to update pillar: %w", err)
	}

	_, err = es.createHosts(ctx, session, creator, cluster.ClusterID, masterSubCluster.SubClusterID, hsts, subnets, resourcePreset, diskTypeExtID, diskSize, cluster.Revision)
	return err
}

type creatorModifier interface {
	AddHosts(ctx context.Context, args []models.AddHostArgs) ([]hosts.Host, error)
	GenerateFQDN(geoName string, vType environment.VType, platform compute.Platform) (string, error)
}

func (es *OpenSearch) createHosts(
	ctx context.Context,
	session sessions.Session,
	modifier creatorModifier,
	cid, subcid string,
	hsts []osmodels.Host,
	subnets []networkProvider.Subnet, resourcePreset resources.Preset, diskTypeExtID string, diskSize int64, revision int64) ([]hosts.Host, error) {
	var hs []models.AddHostArgs

	for _, host := range hsts {
		fqdn, err := modifier.GenerateFQDN(host.ZoneID, resourcePreset.VType, compute.Ubuntu)
		if err != nil {
			return nil, err
		}

		subnet, err := es.compute.PickSubnet(ctx, subnets, resourcePreset.VType, host.ZoneID, host.AssignPublicIP, host.SubnetID, session.FolderCoords.FolderExtID)
		if err != nil {
			return nil, err
		}

		hs = append(hs, models.AddHostArgs{
			ClusterID:        cid,
			SubClusterID:     subcid,
			ResourcePresetID: resourcePreset.ID,
			ZoneID:           host.ZoneID,
			FQDN:             fqdn,
			DiskTypeExtID:    diskTypeExtID,
			SpaceLimit:       diskSize,
			SubnetID:         subnet.ID,
			AssignPublicIP:   host.AssignPublicIP,
			Revision:         revision,
		})
	}

	return modifier.AddHosts(ctx, hs)
}

func (es *OpenSearch) validateServiceAccountAccess(ctx context.Context, session sessions.Session, serviceAccountID string, vtype environment.VType) error {
	if vtype != environment.VTypeCompute {
		return semerr.FailedPrecondition("service accounts not supported in porto clusters")
	}
	return session.ValidateServiceAccount(ctx, serviceAccountID)
}

func splitHosts(hsts []osmodels.Host) (osmodels.ClusterHosts, error) {
	var clusterHosts osmodels.ClusterHosts
	for _, host := range hsts {
		switch host.Role {
		case hosts.RoleOpenSearchDataNode:
			clusterHosts.DataNodes = append(clusterHosts.DataNodes, host)
		case hosts.RoleOpenSearchMasterNode:
			clusterHosts.MasterNodes = append(clusterHosts.MasterNodes, host)
		default:
			return osmodels.ClusterHosts{}, xerrors.Errorf("Unexpected host type: %s", host.Role)
		}
	}
	return clusterHosts, nil
}

func (es *OpenSearch) toOpenSearchModel(ctx context.Context, reader clusterslogic.Reader, clExt clusters.ClusterExtended) (osmodels.Cluster, error) {
	var result osmodels.Cluster
	pillar := ospillars.NewCluster()
	if err := pillar.UnmarshalPillar(clExt.Pillar); err != nil {
		return osmodels.Cluster{}, err
	}
	dataResources, err := reader.ResourcesByClusterIDRoleAtRevision(ctx, clExt.ClusterID, clExt.Revision, hosts.RoleOpenSearchDataNode)
	if err != nil {
		return osmodels.Cluster{}, err
	}

	defaultDataNodeConfig := &ospillars.NewDataNodeSubCluster().Data.OpenSearch.Config.DataNode
	userDataPillar := &ospillars.DataNodeSubCluster{}
	_, err = reader.SubClusterByRole(ctx, clExt.ClusterID, hosts.RoleOpenSearchDataNode, userDataPillar)
	if err != nil {
		return osmodels.Cluster{}, err
	}
	effectiveDataNodeConfig, err := defaultDataNodeConfig.Merge(&userDataPillar.Data.OpenSearch.Config.DataNode)
	if err != nil {
		return osmodels.Cluster{}, err
	}

	masterNode := osmodels.OptionalMasterNode{}
	if !userDataPillar.Data.OpenSearch.IsMaster {
		masterResources, err := reader.ResourcesByClusterIDRoleAtRevision(ctx, clExt.ClusterID, clExt.Revision, hosts.RoleOpenSearchMasterNode)
		if err != nil {
			return osmodels.Cluster{}, err
		}
		masterNode.Set(osmodels.MasterNode{
			Resources: masterResources,
		})
	}

	result.ClusterExtended = clExt
	config := osmodels.Config{
		MasterNode: masterNode,
		DataNode: osmodels.DataNode{
			Resources: dataResources,
			ConfigSet: osmodels.DataNodeConfigSet{
				EffectiveConfig: effectiveDataNodeConfig,
				DefaultConfig:   *defaultDataNodeConfig,
				UserConfig:      userDataPillar.Data.OpenSearch.Config.DataNode,
			},
		},
		Plugins: pillar.Data.OpenSearch.Plugins,
	}
	result.ServiceAccountID = pillar.Data.ServiceAccountID
	access := clusters.Access{}
	if pillar.Data.Access != nil {
		access.DataTransfer = optional.NewBool(*pillar.Data.Access.DataTransfer)
	}
	result.Config = osmodels.ClusterConfig{
		Version: pillar.Version(),
		Config:  config,
		Access:  access,
	}
	return result, nil
}

func (es *OpenSearch) BackupCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return es.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeOpenSearch,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {

			op, err := es.tasks.BackupCluster(
				ctx,
				session,
				cid,
				cluster.Revision,
				osmodels.TaskTypeClusterBackup,
				osmodels.OperationTypeClusterBackup,
				osmodels.MetadataBackupCluster{},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
		clusterslogic.WithPermission(osmodels.PermClustersUpdate),
	)
}

func (es *OpenSearch) RescheduleMaintenance(ctx context.Context, cid string, rescheduleType clusters.RescheduleType, delayedUntil optional.Time) (operations.Operation, error) {
	f := func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
		mntTime, err := modifier.RescheduleMaintenance(ctx, cluster.ClusterID, rescheduleType, delayedUntil)
		if err != nil {
			return operations.Operation{}, err
		}

		op, err := es.tasks.CreateFinishedTask(
			ctx,
			session,
			cluster.ClusterID,
			cluster.Revision,
			osmodels.OperationTypeMaintenanceReschedule,
			osmodels.MetadataRescheduleMaintenance{
				DelayedUntil: mntTime,
			},
			true,
		)
		if err != nil {
			return operations.Operation{}, err
		}

		return op, nil
	}

	return es.operator.ModifyOnClusterWithoutRevChanging(ctx, cid, clusters.TypeOpenSearch, f)
}

func (es *OpenSearch) RestoreCluster(ctx context.Context, args opensearch.RestoreClusterArgs) (operations.Operation, error) {
	// check existence and access to backup
	_, err := es.Backup(ctx, args.RestoreFrom)
	if err != nil {
		return operations.Operation{}, err
	}

	return es.CreateCluster(ctx, args.CreateClusterArgs)
}
