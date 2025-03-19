package provider

import (
	"context"
	"sort"
	"time"

	compute_platform "a.yandex-team.ru/cloud/mdb/internal/compute"
	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	taskslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/provider/internal/sspillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/ssmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

const (
	sqlServerService = "managed-sqlserver"
)

func (ss *SQLServer) Cluster(ctx context.Context, cid string) (ssmodels.Cluster, error) {
	var res ssmodels.Cluster
	if err := ss.operator.ReadCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {

			clExtended, err := reader.ClusterExtendedByClusterID(ctx, cid, clusters.TypeSQLServer, models.VisibilityVisible, session)
			if err != nil {
				return err
			}

			ssmodel, err := ss.toSQLServerModel(ctx, reader, clExtended)
			if err != nil {
				return err
			}
			res = ssmodel

			return nil
		},
	); err != nil {
		return ssmodels.Cluster{}, err
	}
	return res, nil
}

func (ss *SQLServer) Clusters(ctx context.Context, folderExtID string, limit, offset int64) ([]ssmodels.Cluster, error) {
	var res []ssmodels.Cluster
	if err := ss.operator.ReadOnFolder(ctx, folderExtID,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			clsExtended, err := reader.ClustersExtended(ctx, models.ListClusterArgs{
				ClusterType: clusters.TypeSQLServer,
				FolderID:    session.FolderCoords.FolderID,
				Limit:       optional.NewInt64(limit),
				Offset:      offset,
				Visibility:  models.VisibilityVisible,
			}, session)
			if err != nil {
				return err
			}

			res = make([]ssmodels.Cluster, 0, len(clsExtended))
			for _, cl := range clsExtended {
				ssmodel, err := ss.toSQLServerModel(ctx, reader, cl)
				if err != nil {
					return err
				}
				res = append(res, ssmodel)
			}
			return nil
		},
	); err != nil {
		return nil, err
	}
	return res, nil
}

func (ss *SQLServer) toSQLServerModel(ctx context.Context, reader clusterslogic.Reader, clExtended clusters.ClusterExtended) (ssmodels.Cluster, error) {
	var res ssmodels.Cluster

	userPillar := sspillars.NewCluster()
	if err := userPillar.UnmarshalPillar(clExtended.Pillar); err != nil {
		return ssmodels.Cluster{}, err
	}

	defaultPillar := sspillars.NewClusterWithVersion(userPillar.Data.SQLServer.Version)
	err := reader.ClusterTypePillar(ctx, clusters.TypeSQLServer, defaultPillar)
	if err != nil {
		return ssmodels.Cluster{}, err
	}

	resources, err := reader.ResourcesByClusterIDRoleAtRevision(ctx, clExtended.ClusterID, clExtended.Revision, hosts.RoleSQLServer)
	if err != nil {
		return ssmodels.Cluster{}, err
	}
	effectiveConfig, err := ssmodels.ClusterConfigMerge(defaultPillar.Data.SQLServer.Config, userPillar.Data.SQLServer.Config)
	if err != nil {
		return ssmodels.Cluster{}, err
	}
	res = ssmodels.Cluster{
		ClusterExtended: clExtended,
		Config: ssmodels.ClusterConfig{
			Version:   userPillar.Data.SQLServer.Version.MajorHuman,
			Resources: resources,
			ConfigSet: ssmodels.ConfigSetSQLServer{
				UserConfig:      userPillar.Data.SQLServer.Config,
				DefaultConfig:   defaultPillar.Data.SQLServer.Config,
				EffectiveConfig: effectiveConfig,
			},
			BackupWindowStart: clExtended.BackupSchedule.Start,
			Access: ssmodels.Access{
				DataLens:     userPillar.Data.Access.DataLens,
				WebSQL:       userPillar.Data.Access.WebSQL,
				DataTransfer: userPillar.Data.Access.DataTransfer,
			},
			SecondaryConnections: ssmodels.SecondaryConnectionsFromUR(userPillar.Data.SQLServer.UnreadableReplicas),
		},
		SQLCollation:     userPillar.Data.SQLServer.SQLCollation,
		ServiceAccountID: userPillar.Data.ServiceAccountID,
	}

	return res, nil
}

func (ss *SQLServer) CreateCluster(ctx context.Context, args sqlserver.CreateClusterArgs) (operations.Operation, error) {
	return ss.operator.Create(ctx, args.FolderExtID,
		func(ctx context.Context, session sessions.Session, creator clusterslogic.Creator) (clusters.Cluster, operations.Operation, error) {
			args.SecurityGroupIDs = slices.DedupStrings(args.SecurityGroupIDs)
			//Validate CreateClusterArgs
			err := args.ValidateCreate()
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			err = session.ValidateServiceAccount(ctx, args.ServiceAccountID)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			productID, err := getProductID(args.ClusterConfigSpec.Version.Must(), ss.cfg.SQLServer)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			err = ss.license.CheckLicenseResult(ctx, session.FolderCoords.CloudExtID, []string{productID})
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			// Create cluster, subcluster and hosts
			cluster, privKey, err := ss.createCluster(ctx, creator, session, args.NewClusterArgs)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			// Create cluster pillar
			pillar, err := ss.makeNewClusterPillar(cluster, privKey, args)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
			}
			err = creator.AddClusterPillar(ctx, cluster.ClusterID, cluster.Revision, pillar)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
			}

			op, err := ss.tasks.CreateCluster(
				ctx,
				session,
				cluster.ClusterID,
				cluster.Revision,
				ssmodels.TaskTypeClusterCreate,
				ssmodels.OperationTypeClusterCreate,
				ssmodels.MetadataCreateCluster{},
				optional.NewString(pillar.Data.S3Bucket),
				args.SecurityGroupIDs,
				sqlServerService,
				searchAttributesExtractor,
				taskslogic.CreateTaskArgs(
					map[string]interface{}{
						"major_version":      pillar.Data.SQLServer.Version.MajorHuman,
						"service_account_id": pillar.Data.ServiceAccountID,
					},
				),
			)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			return cluster, op, nil
		},
	)
}

func getProductID(version string, cfg logic.SQLServerConfig) (string, error) {
	edition := ssmodels.VersionEdition(version)
	switch edition {
	case ssmodels.EditionEnterprise:
		return cfg.ProductIDs.Enterprise, nil
	case ssmodels.EditionStandard:
		return cfg.ProductIDs.Standard, nil
	default:
		return "", xerrors.Errorf("unknown ProductID for version %+v", version)
	}
}

func (ss *SQLServer) initResourcesIfEmpty(resources *models.ClusterResources, role hosts.Role) error {
	return ss.console.InitResourcesIfEmpty(resources, role, clusters.TypeSQLServer)
}

func searchAttributesExtractor(cluster clusterslogic.Cluster) (map[string]interface{}, error) {
	pillar := sspillars.NewCluster()
	if err := cluster.Pillar(pillar); err != nil {
		return nil, err
	}

	searchAttributes := make(map[string]interface{})

	databases := make([]string, 0, len(pillar.Data.SQLServer.Databases))
	for d := range pillar.Data.SQLServer.Databases {
		databases = append(databases, d)
	}
	sort.Strings(databases)
	searchAttributes["databases"] = databases

	validator := ssmodels.NewSystemsUsersValidator()
	users := make([]string, 0, len(pillar.Data.SQLServer.Users))
	for _, u := range pillar.Users(cluster.ClusterID) {
		if validator.ValidateString(u.Name) != nil {
			continue
		}
		users = append(users, u.Name)
	}
	sort.Strings(users)
	searchAttributes["users"] = users

	return searchAttributes, nil
}

func (ss *SQLServer) toSearchQueue(ctx context.Context, folderCoords metadb.FolderCoords, op operations.Operation) error {
	return ss.search.StoreDoc(
		ctx,
		sqlServerService,
		folderCoords.FolderExtID,
		folderCoords.CloudExtID,
		op,
		searchAttributesExtractor,
	)
}

func (ss *SQLServer) RestoreCluster(ctx context.Context, args sqlserver.RestoreClusterArgs) (operations.Operation, error) {
	return ss.operator.Restore(ctx, args.FolderExtID,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, restorer clusterslogic.Restorer) (clusters.Cluster, operations.Operation, error) {
			sourceCid, backupID, err := bmodels.DecodeGlobalBackupID(args.BackupID)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}
			backup, err := ss.backups.BackupByClusterIDBackupID(ctx, sourceCid, backupID,
				func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
					return ss.listS3BackupImpl(ctx, restorer, client, cid)
				})
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			// load source cluster info
			sourceCluster, sourceResource, err := restorer.ClusterAndResourcesAtTime(ctx,
				sourceCid, args.Time, clusters.TypeSQLServer, hosts.RoleSQLServer)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}
			// TODO: check access to source cluster

			var sourcePillar sspillars.Cluster
			err = sourceCluster.Pillar(&sourcePillar)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			// validate restore posibility
			err = args.ValidateRestore(backup, sourceResource)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			//validate that the versions and editions match

			if args.ClusterConfigSpec.Version.String != sourcePillar.Data.SQLServer.Version.MajorHuman {
				return clusters.Cluster{}, operations.Operation{}, semerr.InvalidInput("SQL Server versions need to match")
			}

			args.SecurityGroupIDs = slices.DedupStrings(args.SecurityGroupIDs)
			// Create cluster, subcluster and hosts
			cluster, privKey, err := ss.createCluster(ctx, restorer, session, args.NewClusterArgs)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			// Save target pillar
			targetPillarID, err := restorer.AddTargetPillar(ctx, cluster.ClusterID, pillars.MakeTargetPillar(sourcePillar.Data))
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			// Create cluster pillar
			pillar, err := ss.makeClusterPillarFromSource(cluster, privKey, sourcePillar, args)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
			}

			err = restorer.AddClusterPillar(ctx, cluster.ClusterID, cluster.Revision, pillar)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
			}

			if len(args.HostGroupIDs) > 0 {
				if !session.FeatureFlags.Has(ssmodels.SQLServerDedicatedHostsFlag) {
					return clusters.Cluster{}, operations.Operation{}, semerr.Authorization("operation is not allowed for this cloud")
				}
				zones := ssmodels.CollectZones(args.HostSpecs)
				err := ss.compute.ValidateHostGroups(ctx, args.HostGroupIDs, session.FolderCoords.FolderExtID, session.FolderCoords.CloudExtID, zones)
				if err != nil {
					return clusters.Cluster{}, operations.Operation{}, err
				}
			}

			restoreOperation, err := ss.tasks.CreateCluster(
				ctx,
				session,
				cluster.ClusterID,
				cluster.Revision,
				ssmodels.TaskTypeClusterRestore,
				ssmodels.OperationTypeClusterRestore,
				ssmodels.MetadataRestoreCluster{
					SourceClusterID: sourceCid,
					BackupID:        backupID,
				},
				optional.NewString(pillar.Data.S3Bucket),
				args.SecurityGroupIDs,
				sqlServerService,
				searchAttributesExtractor,
				taskslogic.CreateTaskArgs(
					map[string]interface{}{
						"major_version":      sourcePillar.Data.SQLServer.Version.MajorHuman,
						"service_account_id": pillar.Data.ServiceAccountID,
						"target-pillar-id":   targetPillarID,
						"restore-from": map[string]interface{}{
							"cid":       sourceCid,
							"backup-id": backup.ID,
							"time":      args.Time.UTC().Format(time.RFC3339),
						},
					},
				),
				// TODO: MDB-16309 better timeout estimation
				taskslogic.CreateTaskTimeout(9*time.Hour),
			)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			if err := ss.toSearchQueue(ctx, session.FolderCoords, restoreOperation); err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			return cluster, restoreOperation, nil
		},
	)
}

func (ss *SQLServer) DeleteCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return ss.operator.Delete(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, cluster clusterslogic.Cluster, _ clusterslogic.Reader) (operations.Operation, error) {
			var pillar sspillars.Cluster
			err := cluster.Pillar(&pillar)
			if err != nil {
				return operations.Operation{}, err
			}

			var s3Buckets = taskslogic.DeleteClusterS3Buckets{
				"backup": pillar.Data.S3Bucket,
			}

			op, err := ss.tasks.DeleteCluster(
				ctx,
				session,
				cluster.ClusterID,
				cluster.Revision,
				taskslogic.DeleteClusterTaskTypes{
					Delete:   ssmodels.TaskTypeClusterDelete,
					Metadata: ssmodels.TaskTypeClusterDeleteMetadata,
					Purge:    ssmodels.TaskTypeClusterPurge,
				},
				ssmodels.OperationTypeClusterDelete,
				s3Buckets,
			)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := ss.search.StoreDocDelete(ctx, sqlServerService, session.FolderCoords.FolderExtID, session.FolderCoords.CloudExtID, op); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
	)
}

func (ss *SQLServer) ListHosts(ctx context.Context, cid string) ([]hosts.HostExtended, error) {
	// TODO: pagination
	res, _, _, err := clusterslogic.ListHosts(ctx, ss.operator, cid, clusters.TypeSQLServer, -1, 0)
	return res, err
}

func (ss *SQLServer) createCluster(ctx context.Context, creator clusterslogic.Creator, session sessions.Session, args sqlserver.NewClusterArgs) (clusters.Cluster, []byte, error) {
	if !session.FeatureFlags.Has(ssmodels.SQLServerClusterFeatureFlag) {
		return clusters.Cluster{}, nil, semerr.Authorization("operation is not allowed for this cloud")
	}
	// just to validate resource preset id
	err := args.ClusterConfigSpec.Resources.Validate(true)
	if err != nil {
		return clusters.Cluster{}, nil, err
	}
	//check if we need witness host
	var witnessRequired bool
	if session.FeatureFlags.Has(ssmodels.SQLServerTwoNodeCluster) &&
		len(args.HostSpecs) == 2 {
		witnessRequired = true
	}

	hostGroup := clusterslogic.HostGroup{
		Role:                   hosts.RoleSQLServer,
		NewResourcePresetExtID: optional.NewString(args.ClusterConfigSpec.Resources.ResourcePresetExtID.String),
		DiskTypeExtID:          args.ClusterConfigSpec.Resources.DiskTypeExtID.String,
		NewDiskSize:            args.ClusterConfigSpec.Resources.DiskSize,
		HostsToAdd:             make([]clusterslogic.ZoneHosts, 0, len(args.HostSpecs)),
	}
	for _, hs := range args.HostSpecs {
		hostGroup.HostsToAdd = append(hostGroup.HostsToAdd, clusterslogic.ZoneHosts{ZoneID: hs.ZoneID, Count: 1})
	}

	resolvedHostGroups, _, err := creator.ValidateResources(
		ctx,
		session,
		clusters.TypeSQLServer,
		hostGroup,
	)
	if err != nil {
		return clusters.Cluster{}, nil, err
	}

	resolvedHostGroup := resolvedHostGroups.Single()

	hostGroupHostType := make(map[string]compute.HostGroupHostType, len(args.HostGroupIDs))
	if len(args.HostGroupIDs) > 0 {
		if !session.FeatureFlags.Has(ssmodels.SQLServerDedicatedHostsFlag) {
			return clusters.Cluster{}, nil, semerr.Authorization("operation is not allowed for this cloud")
		}
		err = ss.compute.ValidateHostGroups(ctx, args.HostGroupIDs, session.FolderCoords.FolderExtID, session.FolderCoords.CloudExtID,
			ssmodels.CollectZones(args.HostSpecs))
		if err != nil {
			return clusters.Cluster{}, nil, err
		}

		hostGroupHostType, err = ss.compute.GetHostGroupHostType(ctx, args.HostGroupIDs)
		if err != nil {
			return clusters.Cluster{}, nil, err
		}
	}

	if err = args.Validate(resolvedHostGroup.TargetResourcePreset(), session, hostGroupHostType); err != nil {
		return clusters.Cluster{}, nil, err
	}

	network, err := ss.compute.Network(ctx, args.NetworkID)
	if err != nil {
		return clusters.Cluster{}, nil, err
	}
	var subnets []networkProvider.Subnet
	if network.ID != "" {
		subnets, err = ss.compute.Subnets(ctx, network)
		if err != nil {
			return clusters.Cluster{}, nil, err
		}
	}

	// Create cluster
	c, privKey, err := creator.CreateCluster(ctx, models.CreateClusterArgs{
		Name:               args.Name,
		ClusterType:        clusters.TypeSQLServer,
		Environment:        args.Environment,
		NetworkID:          args.NetworkID,
		FolderID:           session.FolderCoords.FolderID,
		Description:        args.Description,
		Labels:             args.Labels,
		DeletionProtection: args.DeletionProtection,
		HostGroupIDs:       args.HostGroupIDs,
	})
	if err != nil {
		return clusters.Cluster{}, nil, err
	}

	// Create subcluster
	sc, err := creator.CreateSubCluster(ctx, models.CreateSubClusterArgs{
		ClusterID: c.ClusterID,
		Name:      args.Name,
		Roles:     []hosts.Role{hosts.RoleSQLServer},
		Revision:  c.Revision,
	})
	if err != nil {
		return clusters.Cluster{}, nil, err
	}

	if witnessRequired {
		err = ss.AddWitness(ctx, session, creator, &c, args, subnets, resolvedHostGroup)
		if err != nil {
			return clusters.Cluster{}, nil, err
		}
	}
	// Create hosts
	for _, hs := range args.HostSpecs {
		fqdn, err := creator.GenerateFQDN(hs.ZoneID, resolvedHostGroup.TargetResourcePreset().VType, compute_platform.Windows)
		if err != nil {
			return clusters.Cluster{}, nil, err
		}
		subnet, err := ss.compute.PickSubnet(ctx, subnets, resolvedHostGroup.TargetResourcePreset().VType, hs.ZoneID, hs.AssignPublicIP, hs.SubnetID, session.FolderCoords.FolderExtID)
		if err != nil {
			return clusters.Cluster{}, nil, err
		}
		_, err = creator.AddHosts(ctx, []models.AddHostArgs{{
			SubClusterID:     sc.SubClusterID,
			ResourcePresetID: resolvedHostGroup.TargetResourcePreset().ID,
			SpaceLimit:       args.ClusterConfigSpec.Resources.DiskSize.Int64,
			ZoneID:           hs.ZoneID,
			FQDN:             fqdn,
			DiskTypeExtID:    args.ClusterConfigSpec.Resources.DiskTypeExtID.String,
			SubnetID:         subnet.ID,
			AssignPublicIP:   hs.AssignPublicIP,
			ClusterID:        c.ClusterID,
			Revision:         c.Revision,
		}})
		if err != nil {
			return clusters.Cluster{}, nil, err
		}
	}

	// validate security groups after NetworkID validation
	if len(args.SecurityGroupIDs) > 0 {
		err = ss.compute.ValidateSecurityGroups(ctx, args.SecurityGroupIDs, args.NetworkID)
		if err != nil {
			return clusters.Cluster{}, nil, err
		}
	}

	// add backup schedule
	backupSchedule := bmodels.GetBackupSchedule(ss.cfg.SQLServer.BackupSchedule, args.ClusterConfigSpec.BackupWindowStart)
	if err := ss.backups.AddBackupSchedule(ctx, c.ClusterID, backupSchedule, c.Revision); err != nil {
		return clusters.Cluster{}, nil, err
	}

	return c, privKey, nil
}

func (ss *SQLServer) makeClusterPillar(c clusters.Cluster, privKey []byte) (*sspillars.Cluster, error) {
	pillar := sspillars.NewCluster()
	pillar.Data.S3Bucket = ss.cfg.S3BucketName(c.ClusterID)
	privateKeyEnc, err := ss.cryptoProvider.Encrypt(privKey)
	if err != nil {
		return nil, err
	}
	pillar.Data.ClusterPrivateKey = privateKeyEnc
	return pillar, nil
}

func (ss *SQLServer) makeNewClusterPillar(c clusters.Cluster, privKey []byte, args sqlserver.CreateClusterArgs) (*sspillars.Cluster, error) {
	pillar, err := ss.makeClusterPillar(c, privKey)
	if err != nil {
		return nil, err
	}
	for _, ds := range args.DatabaseSpecs {
		err := pillar.AddDatabase(ds)
		if err != nil {
			return nil, err
		}
	}
	err = pillar.CreateSystemUsers(ss.cryptoProvider)
	if err != nil {
		return nil, err
	}
	for _, us := range args.UserSpecs {
		err := pillar.AddUser(us, ss.cryptoProvider)
		if err != nil {
			return nil, err
		}
	}

	if ss.cfg.E2E.IsClusterE2E(c.Name, args.FolderExtID) {
		pillar.Data.Billing = pillars.NewDisabledBilling()
		pillar.Data.ShipLogs = new(bool)
		pillar.Data.MDBHealth = pillars.NewMDBHealthWithDisabledAggregate()
	}

	pillar.Data.S3Bucket = ss.cfg.S3BucketName(c.ClusterID)
	pillar.Data.SQLServer.Version.Major = args.ClusterConfigSpec.Version.Must()
	pillar.Data.SQLServer.Version.MajorHuman = args.ClusterConfigSpec.Version.Must()
	pillar.Data.SQLServer.Version.Edition = ssmodels.VersionEdition(args.ClusterConfigSpec.Version.Must())
	pillar.Data.SQLServer.Config = args.ClusterConfigSpec.Config.Value
	if args.ClusterConfigSpec.Access.DataLens.Valid {
		pillar.Data.Access.DataLens = args.ClusterConfigSpec.Access.DataLens.Bool
	}
	if args.ClusterConfigSpec.Access.WebSQL.Valid {
		pillar.Data.Access.WebSQL = args.ClusterConfigSpec.Access.WebSQL.Bool
	}
	if args.ClusterConfigSpec.Access.DataTransfer.Valid {
		pillar.Data.Access.DataTransfer = args.ClusterConfigSpec.Access.DataTransfer.Bool
	}
	pillar.Data.SQLServer.SQLCollation = args.SQLCollation
	pillar.Data.SQLServer.UnreadableReplicas = ssmodels.SecondaryConnectionsToUR(args.ClusterConfigSpec.SecondaryConnections.GetOrDefault())
	pillar.Data.ServiceAccountID = args.ServiceAccountID

	return pillar, nil
}

func (ss *SQLServer) makeClusterPillarFromSource(c clusters.Cluster, privKey []byte, sourcePillar sspillars.Cluster, args sqlserver.RestoreClusterArgs) (*sspillars.Cluster, error) {
	pillar, err := ss.makeClusterPillar(c, privKey)
	if err != nil {
		return nil, err
	}
	pillar.Data.SQLServer.Version = sourcePillar.Data.SQLServer.Version
	pillar.Data.SQLServer.Databases = sourcePillar.Data.SQLServer.Databases
	pillar.Data.SQLServer.Users = sourcePillar.Data.SQLServer.Users
	pillar.Data.SQLServer.SQLCollation = sourcePillar.Data.SQLServer.SQLCollation
	err = pillar.CreateSystemUsers(ss.cryptoProvider)
	if err != nil {
		return nil, err
	}

	pillar.Data.SQLServer.Config = args.ClusterConfigSpec.Config.Value
	if args.ClusterConfigSpec.Access.DataLens.Valid {
		pillar.Data.Access.DataLens = args.ClusterConfigSpec.Access.DataLens.Bool
	}
	if args.ClusterConfigSpec.Access.WebSQL.Valid {
		pillar.Data.Access.WebSQL = args.ClusterConfigSpec.Access.WebSQL.Bool
	}
	if args.ClusterConfigSpec.Access.DataTransfer.Valid {
		pillar.Data.Access.DataTransfer = args.ClusterConfigSpec.Access.DataTransfer.Bool
	}
	pillar.Data.ServiceAccountID = args.ServiceAccountID
	pillar.Data.SQLServer.UnreadableReplicas = ssmodels.SecondaryConnectionsToUR(args.ClusterConfigSpec.SecondaryConnections.GetOrDefault())

	return pillar, nil
}

func (ss *SQLServer) StartCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return ss.operator.ModifyOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, _ clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			op, err := ss.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      ssmodels.TaskTypeClusterStart,
					OperationType: ssmodels.OperationTypeClusterStart,
					Revision:      cluster.Revision,
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}
			return op, nil
		},
	)
}

func (ss *SQLServer) StopCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return ss.operator.ModifyOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, _ clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			op, err := ss.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      ssmodels.TaskTypeClusterStop,
					OperationType: ssmodels.OperationTypeClusterStop,
					Revision:      cluster.Revision,
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}
			return op, nil
		},
	)
}

func (ss *SQLServer) StartFailover(ctx context.Context, cid string, hostName string) (operations.Operation, error) {
	return ss.operator.ModifyOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			clusterHosts, _, _, err := reader.ListHosts(ctx, cid, -1, 0)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to fetch hosts for cluster %s: %w", cid, err)
			}

			var found bool
			for _, host := range clusterHosts {
				if host.FQDN == hostName {
					found = true
					for _, role := range host.Roles {
						if role == hosts.RoleWindowsWitnessNode {
							return operations.Operation{}, semerr.FailedPrecondition("filover to witness node is prohibited")
						}
					}
					break
				}
			}
			if !found {
				return operations.Operation{}, semerr.FailedPreconditionf("host %s not found in cluster %s", hostName, cid)
			}

			op, err := ss.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      ssmodels.TaskTypeClusterStartFailover,
					OperationType: ssmodels.OperationTypeClusterStartFailover,
					Revision:      cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target_host": hostName,
					},
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}
			return op, nil
		},
	)
}

func (ss *SQLServer) EstimateCreateCluster(ctx context.Context, args sqlserver.CreateClusterArgs) (console.BillingEstimate, error) {
	if err := args.ValidateVersion(); err != nil {
		return console.BillingEstimate{}, err
	}
	var estimate console.BillingEstimate
	err := ss.operator.ReadOnFolder(ctx, args.FolderExtID,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			hostsCount := len(args.HostSpecs)
			hostBillingSpecs := make([]clusterslogic.HostBillingSpec, 0, hostsCount)
			var sqlserverResources models.ClusterResources
			if args.ClusterConfigSpec.Resources.IsSet() {
				if err := args.ClusterConfigSpec.Resources.Validate(true); err != nil {
					return err
				}
				sqlserverResources = args.ClusterConfigSpec.Resources.MustOptionals()
			} else {
				if err := ss.initResourcesIfEmpty(&sqlserverResources, hosts.RoleSQLServer); err != nil {
					return err
				}
			}
			for i := 0; i < hostsCount; i++ {
				hostBillingSpecs = append(hostBillingSpecs, clusterslogic.HostBillingSpec{
					HostRole:         hosts.RoleSQLServer,
					ClusterResources: sqlserverResources,
					AssignPublicIP:   args.HostSpecs[i].AssignPublicIP,
				})
			}
			var err error
			estimate, err = reader.EstimateBilling(ctx, args.FolderExtID, clusters.TypeSQLServer, hostBillingSpecs, environment.CloudTypeYandex)
			if err != nil {
				return err
			}
			edition := ssmodels.VersionEdition(args.ClusterConfigSpec.Version.Must())
			// old format
			for i := range estimate.Metrics {
				estimate.Metrics[i].Tags.Edition = edition
			}
			// new format
			preset, err := ss.console.ResourcePresetByExtID(ctx, args.ClusterConfigSpec.Resources.ResourcePresetExtID.Must())
			if err != nil {
				return err
			}
			var witnessResources models.ClusterResources
			if session.FeatureFlags.Has(ssmodels.SQLServerTwoNodeCluster) &&
				len(args.HostSpecs) == 2 {
				defaultResources, err := ss.cfg.GetDefaultResources(clusters.TypeSQLServer, hosts.RoleWindowsWitnessNode)
				if err != nil {
					return err
				}
				witnessResources = models.ClusterResources{
					ResourcePresetExtID: defaultResources.ResourcePresetExtID,
					DiskTypeExtID:       defaultResources.DiskTypeExtID,
					DiskSize:            defaultResources.DiskSize,
				}
				witnessBillingSpec := make([]clusterslogic.HostBillingSpec, 0, 1)
				witnessBillingSpec = append(witnessBillingSpec, clusterslogic.HostBillingSpec{
					HostRole:         hosts.RoleWindowsWitnessNode,
					ClusterResources: witnessResources,
					AssignPublicIP:   false,
				})
				estimateWitness, err := reader.EstimateBilling(ctx, args.FolderExtID, clusters.TypeSQLServer, witnessBillingSpec, environment.CloudTypeYandex)
				if err != nil {
					return err
				}
				estimateWitness.Metrics[0].Tags.Edition = edition
				estimate.Metrics = append(estimate.Metrics, estimateWitness.Metrics[0])
			}
			coresToBill := int64(preset.CPULimit)
			if coresToBill < ssmodels.MinCoresToBill {
				coresToBill = ssmodels.MinCoresToBill
			}
			if args.ClusterConfigSpec.SecondaryConnections.GetOrDefault() == ssmodels.SecondaryConnectionsReadOnly {
				coresToBill = coresToBill * int64(len(args.HostSpecs))
			}
			estimate.Metrics = append(estimate.Metrics, console.BillingMetric{
				Schema:   console.BillingLicenseSchemaName,
				FolderID: args.FolderExtID,
				Tags: console.BillingMetricTags{
					Roles:        []string{hosts.RoleSQLServer.Stringified()},
					Edition:      edition,
					Cores:        coresToBill,
					CoreFraction: int64(preset.CPUFraction),
				},
			})
			return err
		},
	)
	return estimate, err
}

func (ss *SQLServer) RestoreHints(ctx context.Context, globalBackupID string) (ssmodels.RestoreHints, error) {
	cid, backupID, err := bmodels.DecodeGlobalBackupID(globalBackupID)
	if err != nil {
		return ssmodels.RestoreHints{}, err
	}

	var hints ssmodels.RestoreHints
	if err = ss.operator.ReadOnDeletedCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			backup, err := ss.backups.BackupByClusterIDBackupID(ctx, cid, backupID,
				func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
					return ss.listS3BackupImpl(ctx, reader, client, cid)
				})
			if err != nil {
				return err
			}

			restoreTime := backup.CreatedAt.Add(time.Minute)
			sourceCluster, sourceResource, err := reader.ClusterAndResourcesAtTime(ctx,
				cid, restoreTime, clusters.TypeSQLServer, hosts.RoleSQLServer)
			if err != nil {
				return err
			}

			var sourcePillar sspillars.Cluster
			err = sourceCluster.Pillar(&sourcePillar)
			if err != nil {
				return err
			}

			hints = ssmodels.RestoreHints{
				RestoreHints: console.RestoreHints{
					Environment: sourceCluster.Environment,
					NetworkID:   sourceCluster.NetworkID,
					Version:     sourcePillar.Data.SQLServer.Version.MajorHuman,
					Time:        restoreTime,
				},
				Resources: console.RestoreResources{
					ResourcePresetID: sourceResource.ResourcePresetExtID,
					DiskSize:         sourceResource.DiskSize,
				},
			}
			return nil
		},
	); err != nil {
		return ssmodels.RestoreHints{}, err
	}

	return hints, nil
}

func getZoneForWitness(ctx context.Context, session sessions.Session, creator clusterslogic.Creator, hostSpecs []ssmodels.HostSpec) ([]string, error) {
	availableZones, err := creator.ListAvailableZones(ctx, session, true)
	if err != nil {
		return nil, err
	}
	var chosenZones []string
	for _, zone := range hostSpecs {
		chosenZones = append(chosenZones, zone.ZoneID)
	}
	var witnessZones []string
	for _, zone := range availableZones {
		if match, err := slices.Contains(chosenZones, zone.Name); err != nil {
			return nil, err
		} else if !match {
			witnessZones = append(witnessZones, zone.Name)
		}
	}
	return witnessZones, nil
}

func (ss *SQLServer) AddWitness(ctx context.Context, session sessions.Session, creator clusterslogic.Creator, c *clusters.Cluster, args sqlserver.NewClusterArgs, subnets []networkProvider.Subnet, resolvedHostGroup clusterslogic.ResolvedHostGroup) error {

	witnessZones, err := getZoneForWitness(ctx, session, creator, args.HostSpecs)
	if err != nil {
		return err
	}

	fqdn, err := creator.GenerateFQDN(witnessZones[0], resolvedHostGroup.TargetResourcePreset().VType, compute_platform.Windows)
	if err != nil {
		return err
	}

	subnet, err := ss.compute.PickSubnet(ctx, subnets, resolvedHostGroup.TargetResourcePreset().VType, witnessZones[0], false, optional.String{}, session.FolderCoords.FolderExtID)
	if err != nil {
		return err
	}

	defaultResources, err := ss.cfg.GetDefaultResources(clusters.TypeSQLServer, hosts.RoleWindowsWitnessNode)
	if err != nil {
		return err
	}
	witnessResources := models.ClusterResources{
		ResourcePresetExtID: defaultResources.ResourcePresetExtID,
		DiskTypeExtID:       defaultResources.DiskTypeExtID,
		DiskSize:            defaultResources.DiskSize,
	}

	WitnessHostGroup := clusterslogic.HostGroup{
		Role:                   hosts.RoleWindowsWitnessNode,
		NewResourcePresetExtID: optional.NewString(witnessResources.ResourcePresetExtID),
		DiskTypeExtID:          witnessResources.DiskTypeExtID,
		NewDiskSize:            optional.NewInt64(witnessResources.DiskSize),
		HostsToAdd:             make([]clusterslogic.ZoneHosts, 0, 1),
	}

	WitnessHostGroup.HostsToAdd = append(WitnessHostGroup.HostsToAdd, clusterslogic.ZoneHosts{ZoneID: witnessZones[0], Count: 1})

	WitnessResolvedHostGroups, _, err := creator.ValidateResources(
		ctx,
		session,
		clusters.TypeSQLServer,
		WitnessHostGroup,
	)
	if err != nil {
		return err
	}

	WitnessResolvedHostGroup := WitnessResolvedHostGroups.Single()

	if err = args.Validate(WitnessResolvedHostGroup.TargetResourcePreset(), session, nil); err != nil {
		return err
	}

	//Create witness subcluster
	wsc, err := creator.CreateSubCluster(ctx, models.CreateSubClusterArgs{
		ClusterID: c.ClusterID,
		Name:      args.Name + "_witness",
		Roles:     []hosts.Role{hosts.RoleWindowsWitnessNode},
		Revision:  c.Revision,
	})
	if err != nil {
		return err
	}
	witnessResourcePreset, err := ss.console.ResourcePresetByExtID(ctx, witnessResources.ResourcePresetExtID)
	if err != nil {
		return err
	}
	_, err = creator.AddHosts(ctx, []models.AddHostArgs{{
		SubClusterID:     wsc.SubClusterID,
		ResourcePresetID: witnessResourcePreset.ID,
		SpaceLimit:       witnessResources.DiskSize,
		ZoneID:           witnessZones[0],
		FQDN:             fqdn,
		DiskTypeExtID:    witnessResources.DiskTypeExtID,
		SubnetID:         subnet.ID,
		AssignPublicIP:   false,
		ClusterID:        c.ClusterID,
		Revision:         c.Revision,
	}})
	if err != nil {
		return err
	}
	return nil
}

func (ss *SQLServer) GetHostGroupType(ctx context.Context, hostGroupIds []string) (map[string]compute.HostGroupHostType, error) {

	return ss.compute.GetHostGroupHostType(ctx, hostGroupIds)

}
