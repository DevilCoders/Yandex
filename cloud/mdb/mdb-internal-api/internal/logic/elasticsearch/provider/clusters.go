package provider

import (
	"context"
	"sort"

	"a.yandex-team.ru/cloud/mdb/internal/compute"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/esmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/provider/internal/espillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	taskslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
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
	elasticSearchService     = "managed-elasticsearch"
	kibanaEncKeyLen          = 32
)

func (es *ElasticSearch) Cluster(ctx context.Context, cid string) (esmodels.Cluster, error) {
	var cl esmodels.Cluster
	if err := es.operator.ReadCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			clExt, err := reader.ClusterExtendedByClusterID(ctx, cid, clusters.TypeElasticSearch, models.VisibilityVisible, session)
			if err != nil {
				return err
			}
			cl, err = es.toElasticsearchModel(ctx, reader, clExt)
			return err
		},
		clusterslogic.WithPermission(esmodels.PermClustersGet),
	); err != nil {
		return esmodels.Cluster{}, err
	}

	return cl, nil
}

func (es *ElasticSearch) Clusters(ctx context.Context, folderExtID string, limit, offset int64) ([]esmodels.Cluster, error) {
	var cls []esmodels.Cluster
	if err := es.operator.ReadOnFolder(ctx, folderExtID,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			var err error
			clsExt, err := reader.ClustersExtended(ctx, models.ListClusterArgs{
				ClusterType: clusters.TypeElasticSearch,
				FolderID:    session.FolderCoords.FolderID,
				Limit:       optional.NewInt64(limit),
				Offset:      offset,
				Visibility:  models.VisibilityVisible,
			}, session)
			if err != nil {
				return err
			}

			for _, clExt := range clsExt {
				cl, err := es.toElasticsearchModel(ctx, reader, clExt)
				if err != nil {
					return err
				}
				cls = append(cls, cl)
			}
			return nil
		},
		clusterslogic.WithPermission(esmodels.PermClustersGet),
	); err != nil {
		return nil, err
	}

	return cls, nil
}

func (es *ElasticSearch) CreateCluster(ctx context.Context, args elasticsearch.CreateClusterArgs) (operations.Operation, error) {
	return es.operator.Create(ctx, args.FolderExtID,
		func(ctx context.Context, session sessions.Session, creator clusterslogic.Creator) (clusters.Cluster, operations.Operation, error) {
			if err := models.ClusterNameValidator.ValidateString(args.Name); err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}
			for _, user := range args.UserSpecs {
				if err := user.Validate(); err != nil {
					return clusters.Cluster{}, operations.Operation{}, err
				}
			}

			// TODO: fix when UI and CLI (CLOUDFRONT-6265) are deployed
			if len(args.UserSpecs) < 1 && !args.ConfigSpec.AdminPassword.Valid {
				return clusters.Cluster{}, operations.Operation{}, semerr.FailedPrecondition("at least 1 user is required")
			}

			//if !args.ConfigSpec.AdminPassword.Valid {
			//	return clusters.Cluster{}, operations.Operation{}, semerr.FailedPrecondition("admin password must be specified")
			//}

			if args.ConfigSpec.AdminPassword.Valid {
				if err := esmodels.ValidateAdminPassword(args.ConfigSpec.AdminPassword.Value); err != nil {
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
					Role:                   hosts.RoleElasticSearchDataNode,
					NewResourcePresetExtID: optional.NewString(res.ResourcePresetExtID),
					DiskTypeExtID:          res.DiskTypeExtID,
					NewDiskSize:            optional.NewInt64(res.DiskSize),
					HostsToAdd:             make([]clusterslogic.ZoneHosts, 0, len(clusterHosts.DataNodes)),
					SkipValidations: clusterslogic.SkipValidations{
						MaxHosts: session.FeatureFlags.Has(esmodels.ElasticSearchAllowUnlimitedHostsFeatureFlag),
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
					Role:                   hosts.RoleElasticSearchMasterNode,
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
				clusters.TypeElasticSearch,
				hostGroups...,
			)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			if args.ServiceAccountID != "" {
				resourcePreset := resolvedHostGroups.MustByHostRole(hosts.RoleElasticSearchDataNode).TargetResourcePreset()
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
			dataPillar := espillars.NewDataNodeSubCluster()
			dataPillar.Data.ElasticSearch.Config.DataNode = config.DataNode.Config
			if len(clusterHosts.MasterNodes) == 0 {
				dataPillar.Data.ElasticSearch.IsMaster = true
			}

			dataNodeResources := config.DataNode.Resources.MustOptionals()
			err = es.createDataNodeSubcluster(
				ctx,
				session,
				creator,
				cluster,
				clusterHosts.DataNodes,
				subnets,
				resolvedHostGroups.MustByHostRole(hosts.RoleElasticSearchDataNode).TargetResourcePreset(),
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
					resolvedHostGroups.MustByHostRole(hosts.RoleElasticSearchMasterNode).TargetResourcePreset(),
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
					esmodels.TaskTypeClusterCreate,
					esmodels.OperationTypeClusterCreate,
					esmodels.MetadataCreateCluster{},
					optional.String{}, // We pass additional buckets via options
					args.SecurityGroupIDs,
					elasticSearchService,
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
				esmodels.TaskTypeClusterRestore,
				esmodels.OperationTypeClusterRestore,
				esmodels.MetadataRestoreCluster{BackupID: args.RestoreFrom},
				optional.String{}, // We pass additional buckets via options
				args.SecurityGroupIDs,
				elasticSearchService,
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
		clusterslogic.WithPermission(esmodels.PermClustersCreate),
	)
}

func (es *ElasticSearch) DeleteCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return es.operator.Delete(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, cluster clusterslogic.Cluster, _ clusterslogic.Reader) (operations.Operation, error) {
			var pillar espillars.Cluster
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
					Delete:   esmodels.TaskTypeClusterDelete,
					Metadata: esmodels.TaskTypeClusterDeleteMetadata,
					Purge:    esmodels.TaskTypeClusterPurge,
				},
				esmodels.OperationTypeClusterDelete,
				s3Buckets,
			)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := es.search.StoreDocDelete(ctx, elasticSearchService, session.FolderCoords.FolderExtID, session.FolderCoords.CloudExtID, op); err != nil {
				return operations.Operation{}, err
			}
			return op, nil
		},
		clusterslogic.WithPermission(esmodels.PermClustersDelete),
	)
}

func (es *ElasticSearch) StartCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, _ clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			op, err := es.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      esmodels.TaskTypeClusterStart,
					OperationType: esmodels.OperationTypeClusterStart,
					Revision:      cluster.Revision,
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}
			return op, nil
		},
		clusterslogic.WithPermission(esmodels.PermClustersStart),
	)
}

func (es *ElasticSearch) StopCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, _ clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			op, err := es.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      esmodels.TaskTypeClusterStop,
					OperationType: esmodels.OperationTypeClusterStop,
					Revision:      cluster.Revision,
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}
			return op, nil
		},
		clusterslogic.WithPermission(esmodels.PermClustersStop),
	)
}

func (es *ElasticSearch) ListHosts(ctx context.Context, cid string, limit, offset int64) ([]hosts.HostExtended, pagination.OffsetPageToken, error) {
	res, newOffset, hasMore, err := clusterslogic.ListHosts(ctx, es.operator, cid, clusters.TypeElasticSearch, limit, offset, clusterslogic.WithPermission(esmodels.PermClustersGet))
	if err != nil {
		return nil, pagination.OffsetPageToken{}, err
	}

	return res, pagination.OffsetPageToken{
		Offset: newOffset,
		More:   hasMore,
	}, nil
}

func (es *ElasticSearch) AddHosts(ctx context.Context, cid string, hsts []esmodels.Host) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {

			if len(hsts) > 1 {
				return operations.Operation{}, semerr.NotImplemented("adding multiple hosts at once is not supported yet")
			}

			if len(hsts) < 1 {
				return operations.Operation{}, semerr.FailedPrecondition("no hosts to add are specified")
			}

			clusterPillar := espillars.NewCluster()
			if err := cluster.Pillar(clusterPillar); err != nil {
				return operations.Operation{}, err
			}

			hostToAdd := hsts[0]
			if hostToAdd.Role != hosts.RoleElasticSearchDataNode {
				return operations.Operation{}, semerr.NotImplemented("only data nodes can be added")
			}

			currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cluster.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			dataHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleElasticSearchDataNode)
			if len(dataHosts) == 0 {
				return operations.Operation{}, xerrors.New("adding hosts to cluster without data nodes")
			}
			anyDataHost := dataHosts[0]

			hostGroup := clusterslogic.HostGroup{
				Role:                       hosts.RoleElasticSearchDataNode,
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
					MaxHosts: session.FeatureFlags.Has(esmodels.ElasticSearchAllowUnlimitedHostsFeatureFlag),
				},
			}

			resolvedHostGroups, _, err := modifier.ValidateResources(
				ctx,
				session,
				clusters.TypeElasticSearch,
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
				[]esmodels.Host{hostToAdd},
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
					TaskType:      esmodels.TaskTypeHostCreate,
					OperationType: esmodels.OperationTypeHostCreate,
					Revision:      cluster.Revision,
					Metadata: esmodels.MetadataCreateHost{
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
		clusterslogic.WithPermission(esmodels.PermClustersUpdate),
	)
}

func (es *ElasticSearch) DeleteHosts(ctx context.Context, cid string, fqdns []string) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeElasticSearch,
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

			dataHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleElasticSearchDataNode)
			if len(dataHosts) == 1 {
				return operations.Operation{}, semerr.FailedPrecondition("the only data node cannot be removed")
			}
			if len(dataHosts) < 3 {
				return operations.Operation{}, semerr.FailedPrecondition("there must be at least 2 data nodes left")
			}
			anyDataHost := dataHosts[0]

			hostGroup := clusterslogic.HostGroup{
				Role:                       hosts.RoleElasticSearchDataNode,
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
				clusters.TypeElasticSearch,
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
					TaskType:      esmodels.TaskTypeHostDelete,
					OperationType: esmodels.OperationTypeHostDelete,
					Revision:      cluster.Revision,
					Metadata: esmodels.MetadataDeleteHost{
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
		clusterslogic.WithPermission(esmodels.PermClustersUpdate),
	)
}

func searchAttributesExtractor(cluster clusterslogic.Cluster) (map[string]interface{}, error) {
	pillar := espillars.NewCluster()
	if err := cluster.Pillar(pillar); err != nil {
		return nil, err
	}

	searchAttributes := make(map[string]interface{})

	users := make([]string, 0, len(pillar.Data.ElasticSearch.Users))
	for name, props := range pillar.Data.ElasticSearch.Users {
		if props.Internal {
			continue
		}
		users = append(users, name)
	}
	sort.Strings(users)
	searchAttributes["users"] = users

	return searchAttributes, nil
}

func (es *ElasticSearch) toSearchQueue(ctx context.Context, folderCoords metadb.FolderCoords, op operations.Operation) error {
	return es.search.StoreDoc(
		ctx,
		elasticSearchService,
		folderCoords.FolderExtID,
		folderCoords.CloudExtID,
		op,
		searchAttributesExtractor,
	)
}

func (es *ElasticSearch) createSystemUser(name string) (espillars.UserData, error) {
	passwordEnc, err := crypto.GenerateEncryptedPassword(es.cryptoProvider, passwordGenLen, nil)
	if err != nil {
		return espillars.UserData{}, err
	}
	return espillars.UserData{
		Name:     name,
		Password: passwordEnc,
		Internal: true,
	}, nil
}

func (es *ElasticSearch) createClusterPillar(args elasticsearch.CreateClusterArgs, clusterID string, privateKey []byte) (*espillars.Cluster, error) {
	// Create cluster pillar
	pillar := espillars.NewCluster()

	defaultVersion, err := es.supportedVersions.DefaultVersion()
	if err != nil {
		return nil, err
	}
	pillar.Data.ElasticSearch.Version = args.ConfigSpec.Version.GetDefault(defaultVersion)
	edition := args.ConfigSpec.Edition.GetDefault(esmodels.DefaultEdition)
	if err := esmodels.CheckAllowedEdition(edition, es.allowedEditions); err != nil {
		return nil, err
	}
	pillar.Data.ElasticSearch.Edition = edition

	pillar.Data.ServiceAccountID = args.ServiceAccountID
	pillar.Data.ElasticSearch.AutoBackups.Enabled = es.cfg.Elasticsearch.EnableAutoBackups

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

	enckey, err := crypto.GenerateEncryptedPassword(es.cryptoProvider, kibanaEncKeyLen, nil)
	if err != nil {
		return nil, err
	}
	pillar.Data.ElasticSearch.Kibana.EncryptionKey = &enckey

	senckey, err := crypto.GenerateEncryptedPassword(es.cryptoProvider, kibanaEncKeyLen, nil)
	if err != nil {
		return nil, err
	}
	pillar.Data.ElasticSearch.Kibana.SavedObjects.EncryptionKey = &senckey

	reportingEncKey, err := crypto.GenerateEncryptedPassword(es.cryptoProvider, kibanaEncKeyLen, nil)
	if err != nil {
		return nil, err
	}
	pillar.Data.ElasticSearch.Kibana.Reporting.EncryptionKey = &reportingEncKey

	if es.cfg.E2E.IsClusterE2E(args.Name, args.FolderExtID) {
		pillar.Data.MDBMetrics = pillars.NewDisabledMDBMetrics()
		pillar.Data.Billing = pillars.NewDisabledBilling()
		pillar.Data.UseYASMAgent = new(bool)
		pillar.Data.SuppressExternalYASMAgent = true
		pillar.Data.ShipLogs = new(bool)
		pillar.Data.MDBHealth = pillars.NewMDBHealthWithDisabledAggregate()
	}

	for _, sysUser := range esmodels.SystemUsers {
		userdata, err := es.createSystemUser(sysUser)
		if err != nil {
			return nil, err
		}
		err = pillar.AddUser(userdata)
		if err != nil {
			return nil, err
		}
	}

	// TODO: fix when UI and CLI (CLOUDFRONT-6265) are deployed
	if args.ConfigSpec.AdminPassword.Valid {
		adminPasswordEnc, err := es.cryptoProvider.Encrypt([]byte(args.ConfigSpec.AdminPassword.Value.Unmask()))
		if err != nil {
			return nil, err
		}
		err = pillar.AddUser(espillars.UserData{
			Name:     esmodels.UserAdmin,
			Password: adminPasswordEnc,
			Internal: false,
		})
		if err != nil {
			return nil, xerrors.Errorf("failed to add user %q operation: %w", esmodels.UserAdmin, err)
		}
	}
	for _, user := range args.UserSpecs {
		userPasswordEnc, err := es.cryptoProvider.Encrypt([]byte(user.Password.Unmask()))
		if err != nil {
			return nil, err
		}
		err = pillar.AddUser(espillars.UserData{
			Name:     user.Name,
			Password: userPasswordEnc,
			Internal: false,
			Roles:    []string{esmodels.RoleAdmin},
		})
		if err != nil {
			return nil, xerrors.Errorf("failed to add user %q operation: %w", user.Name, err)
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

func (es *ElasticSearch) createCluster(ctx context.Context, creator clusterslogic.Creator, args elasticsearch.CreateClusterArgs, folderID int64, network networkProvider.Network) (clusters.Cluster, *espillars.Cluster, error) {
	cluster, privateKey, err := creator.CreateCluster(ctx, models.CreateClusterArgs{
		Name:               args.Name,
		ClusterType:        clusters.TypeElasticSearch,
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

func (es *ElasticSearch) createDataNodeSubcluster(
	ctx context.Context,
	session sessions.Session,
	creator clusterslogic.Creator,
	cluster clusters.Cluster,
	hsts []esmodels.Host,
	subnets []networkProvider.Subnet, resourcePreset resources.Preset, diskTypeExtID string, diskSize int64, pillar *espillars.DataNodeSubCluster) error {
	dataSubCluster, err := creator.CreateSubCluster(ctx, models.CreateSubClusterArgs{
		ClusterID: cluster.ClusterID,
		Name:      dataNodeSubClusterName,
		Roles:     []hosts.Role{hosts.RoleElasticSearchDataNode},
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

func (es *ElasticSearch) createMasterNodeSubcluster(
	ctx context.Context,
	session sessions.Session,
	creator clusterslogic.Creator,
	cluster clusters.Cluster,
	hsts []esmodels.Host,
	subnets []networkProvider.Subnet, resourcePreset resources.Preset, diskTypeExtID string, diskSize int64) error {
	masterSubCluster, err := creator.CreateSubCluster(ctx, models.CreateSubClusterArgs{
		ClusterID: cluster.ClusterID,
		Name:      masterNodeSubClusterName,
		Roles:     []hosts.Role{hosts.RoleElasticSearchMasterNode},
		Revision:  cluster.Revision,
	})
	if err != nil {
		return err
	}

	// Create subcluster pillar
	pillar := espillars.NewMasterNodeSubCluster()
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

func (es *ElasticSearch) createHosts(
	ctx context.Context,
	session sessions.Session,
	modifier creatorModifier,
	cid, subcid string,
	hsts []esmodels.Host,
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

func (es *ElasticSearch) validateServiceAccountAccess(ctx context.Context, session sessions.Session, serviceAccountID string, vtype environment.VType) error {
	if vtype != environment.VTypeCompute {
		return semerr.FailedPrecondition("service accounts not supported in porto clusters")
	}
	return session.ValidateServiceAccount(ctx, serviceAccountID)
}

func splitHosts(hsts []esmodels.Host) (esmodels.ClusterHosts, error) {
	var clusterHosts esmodels.ClusterHosts
	for _, host := range hsts {
		switch host.Role {
		case hosts.RoleElasticSearchDataNode:
			clusterHosts.DataNodes = append(clusterHosts.DataNodes, host)
		case hosts.RoleElasticSearchMasterNode:
			clusterHosts.MasterNodes = append(clusterHosts.MasterNodes, host)
		default:
			return esmodels.ClusterHosts{}, xerrors.Errorf("Unexpected host type: %s", host.Role)
		}
	}
	return clusterHosts, nil
}

func (es *ElasticSearch) toElasticsearchModel(ctx context.Context, reader clusterslogic.Reader, clExt clusters.ClusterExtended) (esmodels.Cluster, error) {
	var result esmodels.Cluster
	pillar := espillars.NewCluster()
	if err := pillar.UnmarshalPillar(clExt.Pillar); err != nil {
		return esmodels.Cluster{}, err
	}
	dataResources, err := reader.ResourcesByClusterIDRoleAtRevision(ctx, clExt.ClusterID, clExt.Revision, hosts.RoleElasticSearchDataNode)
	if err != nil {
		return esmodels.Cluster{}, err
	}

	defaultDataNodeConfig := &espillars.NewDataNodeSubCluster().Data.ElasticSearch.Config.DataNode
	userDataPillar := &espillars.DataNodeSubCluster{}
	_, err = reader.SubClusterByRole(ctx, clExt.ClusterID, hosts.RoleElasticSearchDataNode, userDataPillar)
	if err != nil {
		return esmodels.Cluster{}, err
	}
	effectiveDataNodeConfig, err := defaultDataNodeConfig.Merge(&userDataPillar.Data.ElasticSearch.Config.DataNode)
	if err != nil {
		return esmodels.Cluster{}, err
	}

	masterNode := esmodels.OptionalMasterNode{}
	if !userDataPillar.Data.ElasticSearch.IsMaster {
		masterResources, err := reader.ResourcesByClusterIDRoleAtRevision(ctx, clExt.ClusterID, clExt.Revision, hosts.RoleElasticSearchMasterNode)
		if err != nil {
			return esmodels.Cluster{}, err
		}
		masterNode.Set(esmodels.MasterNode{
			Resources: masterResources,
		})
	}

	result.ClusterExtended = clExt
	config := esmodels.Config{
		MasterNode: masterNode,
		DataNode: esmodels.DataNode{
			Resources: dataResources,
			ConfigSet: esmodels.DataNodeConfigSet{
				EffectiveConfig: effectiveDataNodeConfig,
				DefaultConfig:   *defaultDataNodeConfig,
				UserConfig:      userDataPillar.Data.ElasticSearch.Config.DataNode,
			},
		},
		Plugins: pillar.Data.ElasticSearch.Plugins,
	}
	result.ServiceAccountID = pillar.Data.ServiceAccountID
	access := clusters.Access{}
	if pillar.Data.Access != nil {
		access.DataTransfer = optional.NewBool(*pillar.Data.Access.DataTransfer)
	}
	result.Config = esmodels.ClusterConfig{
		Version: pillar.Version(),
		Edition: pillar.Edition(),
		Config:  config,
		Access:  access,
	}
	return result, nil
}

func (es *ElasticSearch) BackupCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return es.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {

			op, err := es.tasks.BackupCluster(
				ctx,
				session,
				cid,
				cluster.Revision,
				esmodels.TaskTypeClusterBackup,
				esmodels.OperationTypeClusterBackup,
				esmodels.MetadataBackupCluster{},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
		clusterslogic.WithPermission(esmodels.PermClustersUpdate),
	)
}

func (es *ElasticSearch) RescheduleMaintenance(ctx context.Context, cid string, rescheduleType clusters.RescheduleType, delayedUntil optional.Time) (operations.Operation, error) {
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
			esmodels.OperationTypeMaintenanceReschedule,
			esmodels.MetadataRescheduleMaintenance{
				DelayedUntil: mntTime,
			},
			true,
		)
		if err != nil {
			return operations.Operation{}, err
		}

		return op, nil
	}

	return es.operator.ModifyOnClusterWithoutRevChanging(ctx, cid, clusters.TypeElasticSearch, f)
}

func (es *ElasticSearch) RestoreCluster(ctx context.Context, args elasticsearch.RestoreClusterArgs) (operations.Operation, error) {
	// check existence and access to backup
	_, err := es.Backup(ctx, args.RestoreFrom)
	if err != nil {
		return operations.Operation{}, err
	}

	return es.CreateCluster(ctx, args.CreateClusterArgs)
}
