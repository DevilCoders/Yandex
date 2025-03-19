package provider

import (
	"context"
	"math"
	"reflect"
	"sort"
	"time"

	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/mdb/internal/compute"
	compute2 "a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum/gpmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum/provider/internal/gppillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	taskslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
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
	greenplumService      = "managed-greenplum"
	masterSubclusterName  = "master_subcluster"
	segmentSubclusterName = "segment_subcluster"

	componentName = "greenplum"

	defaultMountPoint     = "/var/lib/greenplum/data1"
	placementGroupMaxSize = 4
)

func (gp *Greenplum) Cluster(ctx context.Context, cid string) (gpmodels.Cluster, error) {
	var res gpmodels.Cluster
	if err := gp.operator.ReadCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			clExtended, err := reader.ClusterExtendedByClusterID(ctx, cid, clusters.TypeGreenplumCluster, models.VisibilityVisible, session)
			if err != nil {
				return err
			}

			gpmodel, err := gp.toGreenplumModel(ctx, reader, clExtended)
			if err != nil {
				return err
			}
			res = gpmodel

			return nil
		},
	); err != nil {
		return gpmodels.Cluster{}, err
	}
	return res, nil
}

func (gp *Greenplum) Clusters(ctx context.Context, folderExtID string, limit, offset int64) ([]gpmodels.Cluster, error) {
	var res []gpmodels.Cluster
	if err := gp.operator.ReadOnFolder(ctx, folderExtID,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			clsExtended, err := reader.ClustersExtended(ctx, models.ListClusterArgs{
				ClusterType: clusters.TypeGreenplumCluster,
				FolderID:    session.FolderCoords.FolderID,
				Limit:       optional.NewInt64(limit),
				Offset:      offset,
				Visibility:  models.VisibilityVisible,
			}, session)
			if err != nil {
				return err
			}

			res = make([]gpmodels.Cluster, 0, len(clsExtended))
			for _, cl := range clsExtended {
				gpmodel, err := gp.toGreenplumModel(ctx, reader, cl)
				if err != nil {
					return err
				}
				res = append(res, gpmodel)
			}
			return nil
		},
	); err != nil {
		return nil, err
	}
	return res, nil
}

func (gp *Greenplum) toGreenplumModel(ctx context.Context, reader clusterslogic.Reader, clExtended clusters.ClusterExtended) (gpmodels.Cluster, error) {
	var res gpmodels.Cluster

	userPillar := gppillars.NewCluster()
	if err := userPillar.UnmarshalPillar(clExtended.Pillar); err != nil {
		return gpmodels.Cluster{}, err
	}

	majorVersion, err := getMajorVersion(ctx, reader, clExtended.ClusterID)
	if err != nil {
		return gpmodels.Cluster{}, err
	}

	defaultPillar := gppillars.NewClusterWithVersion(majorVersion)
	err = reader.ClusterTypePillar(ctx, clusters.TypeGreenplumCluster, defaultPillar)
	if err != nil {
		return gpmodels.Cluster{}, err
	}

	resourcesMaster, err := reader.ResourcesByClusterIDRoleAtRevision(ctx, clExtended.ClusterID, clExtended.Revision, hosts.RoleGreenplumMasterNode)
	if err != nil {
		return gpmodels.Cluster{}, err
	}

	resourcesSegment, err := reader.ResourcesByClusterIDRoleAtRevision(ctx, clExtended.ClusterID, clExtended.Revision, hosts.RoleGreenplumSegmentNode)
	if err != nil {
		return gpmodels.Cluster{}, err
	}

	effectiveMasterConfig, err := gpmodels.ClusterConfigMasterMerge(defaultPillar.Data.Greenplum.MasterConfig, userPillar.Data.Greenplum.MasterConfig)

	if err != nil {
		return gpmodels.Cluster{}, err
	}

	effectiveSegmentConfig, err := gpmodels.ClusterConfigSegmentMerge(defaultPillar.Data.Greenplum.SegmentConfig, userPillar.Data.Greenplum.SegmentConfig)
	if err != nil {
		return gpmodels.Cluster{}, err
	}

	effectiveConfig, err := gpmodels.ClusterConfigMerge(defaultPillar.Data.Greenplum.Config, userPillar.Data.Greenplum.Config)
	if err != nil {
		return gpmodels.Cluster{}, err
	}
	effectivePoolConfig, err := gpmodels.PoolConfigMerge(defaultPillar.Data.Pool, userPillar.Data.Pool)
	if err != nil {
		return gpmodels.Cluster{}, err
	}

	res = gpmodels.Cluster{
		ClusterExtended: clExtended,
		ConfigSpec: gpmodels.ClusterGpConfigSet{
			Pooler: gpmodels.PoolerConfigSet{
				UserConfig:      *userPillar.Data.Pool,
				EffectiveConfig: effectivePoolConfig,
				DefaultConfig:   *defaultPillar.Data.Pool,
			},
			Config: gpmodels.ClusterGPDBConfigSet{
				DefaultConfig:   defaultPillar.Data.Greenplum.Config.FromPillar(),
				UserConfig:      userPillar.Data.Greenplum.Config.FromPillar(),
				EffectiveConfig: effectiveConfig.FromPillar()},
		},
		Config: gpmodels.ClusterConfig{
			Version:                majorVersion,
			ZoneID:                 userPillar.Data.Greenplum.Config.ZoneID,
			SubnetID:               userPillar.Data.Greenplum.Config.SubnetID,
			AssignPublicIP:         userPillar.Data.Greenplum.Config.AssignPublicIP,
			SegmentMirroringEnable: userPillar.Data.Greenplum.Config.SegmentMirroringEnable,
			Access: &gpmodels.AccessSettings{
				DataLens:     userPillar.Data.Access.DataLens,
				WebSQL:       userPillar.Data.Access.WebSQL,
				DataTransfer: userPillar.Data.Access.DataTransfer,
				Serverless:   userPillar.Data.Access.Serverless,
			},
		},
		MasterConfig: gpmodels.MasterConfig{
			Resources: resourcesMaster,
			Config: gpmodels.ClusterMasterConfigSet{
				UserConfig:      userPillar.Data.Greenplum.MasterConfig,
				DefaultConfig:   defaultPillar.Data.Greenplum.MasterConfig,
				EffectiveConfig: effectiveMasterConfig,
			},
		},
		SegmentConfig: gpmodels.SegmentConfig{
			Resources: resourcesSegment,
			Config: gpmodels.ClusterSegmentConfigSet{
				UserConfig:      userPillar.Data.Greenplum.SegmentConfig,
				DefaultConfig:   defaultPillar.Data.Greenplum.SegmentConfig,
				EffectiveConfig: effectiveSegmentConfig,
			},
		},
		ClusterConfig: gpmodels.ClusterConfigSet{
			UserConfig:      userPillar.Data.Greenplum.Config,
			DefaultConfig:   defaultPillar.Data.Greenplum.Config,
			EffectiveConfig: effectiveConfig,
		},

		MasterHostCount:  userPillar.Data.Greenplum.MasterHostCount,
		SegmentHostCount: userPillar.Data.Greenplum.SegmentHostCount,
		SegmentInHost:    userPillar.Data.Greenplum.SegmentInHost,
		UserName:         userPillar.Data.Greenplum.AdminUserName,
	}

	return res, nil
}

func (gp *Greenplum) CreateCluster(ctx context.Context, args greenplum.CreateClusterArgs) (operations.Operation, error) {
	return gp.operator.Create(ctx, args.FolderExtID,
		func(ctx context.Context, session sessions.Session, creator clusterslogic.Creator) (clusters.Cluster, operations.Operation, error) {

			args.SecurityGroupIDs = slices.DedupStrings(args.SecurityGroupIDs)
			// Create cluster, subcluster and hosts
			cluster, privKey, segments, err := gp.createCluster(ctx, creator, session, &args, false, false)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			// Create cluster pillar
			pillar, err := gp.makeClusterPillar(cluster, privKey, args, segments)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
			}

			err = pillar.AddUser(args.UserName, args.UserPassword, gp.cryptoProvider)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
			}
			err = gp.fillUsersInPillar(pillar, args.UserName)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
			}

			err = creator.AddClusterPillar(ctx, cluster.ClusterID, cluster.Revision, pillar)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
			}

			op, err := gp.tasks.CreateCluster(
				ctx,
				session,
				cluster.ClusterID,
				cluster.Revision,
				gpmodels.TaskTypeClusterCreate,
				gpmodels.OperationTypeClusterCreate,
				gpmodels.MetadataCreateCluster{},
				optional.NewString(pillar.Data.S3Bucket),
				args.SecurityGroupIDs,
				greenplumService,
				searchAttributesExtractor,
				taskslogic.CreateTaskArgs(
					map[string]interface{}{
						"version": args.Config.Version,
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

func searchAttributesExtractor(cluster clusterslogic.Cluster) (map[string]interface{}, error) {
	pillar := gppillars.NewCluster()
	if err := cluster.Pillar(pillar); err != nil {
		return nil, err
	}

	searchAttributes := make(map[string]interface{})
	users := make([]string, 0, len(pillar.Data.Greenplum.Users))
	for u := range pillar.Data.Greenplum.Users {
		isSystemUser := false
		for _, systemUser := range gppillars.SystemUsers {
			if u == systemUser {
				isSystemUser = true
			}
		}
		if !isSystemUser {
			users = append(users, u)
		}
	}
	sort.Strings(users)
	searchAttributes["users"] = users

	return searchAttributes, nil
}

type hostDisk struct {
	fqdn       string
	mountPoint string
}

func generateSegments(segments map[int]gppillars.SegmentData, startSegment int, hosts []string, SegmentInHost int, segmentMirroringEnable bool, groupSize int,
) (int, map[int][]hostDisk, error) {
	if groupSize == 2 {
		if (len(hosts)*SegmentInHost)%len(hosts) != 0 {
			return startSegment, nil, semerr.InvalidInputf("number of all segments must be multiple of number segment hosts")
		}
	} else {
		if groupSize < 3 && segmentMirroringEnable && len(hosts)%2 == 1 {
			return startSegment, nil, semerr.InvalidInputf("invalid group size; current: %v should be >= 3", groupSize)
		}

		if groupSize < 2 && segmentMirroringEnable {
			return startSegment, nil, semerr.InvalidInputf("invalid group size; current: %v should be >= 2", groupSize)
		}

		if groupSize < 1 {
			return startSegment, nil, semerr.InvalidInputf("invalid group size; current: %v should be >= 1", groupSize)
		}
	}

	groupCount := len(hosts) / groupSize
	if groupSize == 4 {
		if groupCount*groupSize < len(hosts) {
			groupCount++
		}
	}

	groups := make([][]string, groupCount)

	gr := 0
	for _, host := range hosts {
		if gr >= groupCount {
			gr = 0
		}
		groups[gr] = append(groups[gr], host)
		gr++
	}

	hds := make(map[int][]hostDisk, len(groups))
	for i, group := range groups {
		var hdList []hostDisk
		startSegment, hdList = generateSegmentsInGroup(segments, startSegment, group, SegmentInHost, segmentMirroringEnable)
		hds[i] = hdList
	}
	return startSegment, hds, nil
}
func generateSegmentsInGroup(segments map[int]gppillars.SegmentData, startSegment int, hosts []string, SegmentInHost int, segmentMirroringEnable bool) (int, []hostDisk) {
	hdList := make([]hostDisk, 0, len(hosts))
	for i, masterHost := range hosts {
		hdList = append(hdList,
			hostDisk{
				fqdn:       masterHost,
				mountPoint: defaultMountPoint,
			},
		)
		nextReplicaIndex := i + 1
		for k := 0; k < SegmentInHost; k++ {
			segment := gppillars.SegmentData{
				Primary: gppillars.SegmentPlace{
					Fqdn:       masterHost,
					MountPoint: defaultMountPoint,
				},
			}

			if segmentMirroringEnable {
				for {
					if nextReplicaIndex >= len(hosts) {
						nextReplicaIndex = 0
						continue
					}
					if nextReplicaIndex == i {
						nextReplicaIndex++
						continue
					}

					segment.Mirror = &gppillars.SegmentPlace{
						Fqdn:       hosts[nextReplicaIndex],
						MountPoint: defaultMountPoint,
					}
					nextReplicaIndex++
					break
				}
			}

			segments[startSegment] = segment
			startSegment++
		}
	}
	return startSegment, hdList
}

func (gp *Greenplum) IsLowMemSegmentAllowed(ctx context.Context, folderExtID string) (bool, error) {
	var err error
	var hasFlag bool

	err = gp.operator.ReadOnFolder(ctx, folderExtID,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			hasFlag = session.FeatureFlags.Has(gpmodels.GreenplumClusterAllowLowMemSegmentFeatureFlag)
			return nil
		},
	)

	return hasFlag, err
}

func (gp *Greenplum) createCluster(ctx context.Context,
	creator clusterslogic.Creator,
	session sessions.Session,
	args *greenplum.CreateClusterArgs,
	ignoreDeprecatedMajorVersion,
	ignoreUserValidation bool) (clusters.Cluster, []byte, map[int]gppillars.SegmentData, error) {
	if !session.FeatureFlags.Has(gpmodels.GreenplumClusterFeatureFlag) {
		return clusters.Cluster{}, nil, nil, semerr.Authorization("operation is not allowed for this cloud")
	}
	// just to validate resource preset id
	err := args.MasterConfig.Resources.Validate()
	if err != nil {
		return clusters.Cluster{}, nil, nil, err
	}

	err = args.ConfigSpec.Config.Validate()
	if err != nil {
		return clusters.Cluster{}, nil, nil, err
	}

	// just to validate resource preset id
	err = args.SegmentConfig.Resources.Validate()
	if err != nil {
		return clusters.Cluster{}, nil, nil, err
	}

	versions, err := gp.console.GetDefaultVersions(ctx, clusters.TypeGreenplumCluster, args.Environment, componentName)
	if err != nil {
		return clusters.Cluster{}, nil, nil, err
	}

	ver, err := args.GetValidatedVersion(versions, ignoreDeprecatedMajorVersion)
	if err != nil {
		return clusters.Cluster{}, nil, nil, err
	}
	args.Config.Version = ver.MajorVersion

	segmentPreset, err := gp.console.ResourcePresetByExtID(ctx, args.SegmentConfig.Resources.ResourcePresetExtID)
	if err != nil {
		return clusters.Cluster{}, nil, nil, err
	}

	masterPreset, err := gp.console.ResourcePresetByExtID(ctx, args.MasterConfig.Resources.ResourcePresetExtID)
	if err != nil {
		return clusters.Cluster{}, nil, nil, err
	}

	hostGroupHostType := make(map[string]compute2.HostGroupHostType, len(args.HostGroupIDs))
	onDedicatedHost := len(args.HostGroupIDs) > 0
	if onDedicatedHost {
		err = gp.compute.ValidateHostGroups(ctx, args.HostGroupIDs, session.FolderCoords.FolderExtID, session.FolderCoords.CloudExtID, []string{args.Config.ZoneID})
		if err != nil {
			return clusters.Cluster{}, nil, nil, err
		}

		hostGroupHostType, err = gp.GetHostGroupType(ctx, args.HostGroupIDs)
		if err != nil {
			return clusters.Cluster{}, nil, nil, err
		}
	}

	if err = args.Validate(segmentPreset,
		masterPreset,
		session.FeatureFlags.Has(gpmodels.GreenplumClusterAllowLowMemSegmentFeatureFlag), ignoreUserValidation, hostGroupHostType); err != nil {
		return clusters.Cluster{}, nil, nil, err
	}

	hostMasterGroup := clusterslogic.HostGroup{
		Role:                   hosts.RoleGreenplumMasterNode,
		NewResourcePresetExtID: optional.NewString(args.MasterConfig.Resources.ResourcePresetExtID),
		DiskTypeExtID:          args.MasterConfig.Resources.DiskTypeExtID,
		NewDiskSize:            optional.NewInt64(args.MasterConfig.Resources.DiskSize),
		HostsToAdd:             make([]clusterslogic.ZoneHosts, 0, args.MasterHostCount),
		SkipValidations: clusterslogic.SkipValidations{
			DiskSize: onDedicatedHost && args.MasterConfig.Resources.DiskTypeExtID == resources.LocalSSD,
		},
	}
	for i := 0; i < args.MasterHostCount; i++ {
		hostMasterGroup.HostsToAdd = append(hostMasterGroup.HostsToAdd, clusterslogic.ZoneHosts{ZoneID: args.Config.ZoneID, Count: 1})
	}

	hostSegmentGroup := clusterslogic.HostGroup{
		Role:                   hosts.RoleGreenplumSegmentNode,
		NewResourcePresetExtID: optional.NewString(args.SegmentConfig.Resources.ResourcePresetExtID),
		DiskTypeExtID:          args.SegmentConfig.Resources.DiskTypeExtID,
		NewDiskSize:            optional.NewInt64(args.SegmentConfig.Resources.DiskSize),
		HostsToAdd:             make([]clusterslogic.ZoneHosts, 0, args.SegmentHostCount),
		SkipValidations: clusterslogic.SkipValidations{
			DiskSize: onDedicatedHost && args.SegmentConfig.Resources.DiskTypeExtID == resources.LocalSSD,
		},
	}
	for i := 0; i < args.SegmentHostCount; i++ {
		hostSegmentGroup.HostsToAdd = append(hostSegmentGroup.HostsToAdd, clusterslogic.ZoneHosts{ZoneID: args.Config.ZoneID, Count: 1})
	}

	resolvedMasterHostGroups, _, err := creator.ValidateResources(
		ctx,
		session,
		clusters.TypeGreenplumCluster,
		hostMasterGroup,
	)
	if err != nil {
		return clusters.Cluster{}, nil, nil, err
	}

	resolvedSegmentHostGroups, _, err := creator.ValidateResources(
		ctx,
		session,
		clusters.TypeGreenplumCluster,
		hostSegmentGroup,
	)
	if err != nil {
		return clusters.Cluster{}, nil, nil, err
	}

	resolvedMasterHostGroup := resolvedMasterHostGroups.Single()
	resolvedSegmentHostGroup := resolvedSegmentHostGroups.Single()

	network, err := gp.compute.Network(ctx, args.NetworkID)
	if err != nil {
		return clusters.Cluster{}, nil, nil, err
	}
	var subnets []networkProvider.Subnet
	if network.ID != "" {
		subnets, err = gp.compute.Subnets(ctx, network)
		if err != nil {
			return clusters.Cluster{}, nil, nil, err
		}
	}

	// Create cluster
	c, privKey, err := creator.CreateCluster(ctx, models.CreateClusterArgs{
		Name:               args.Name,
		ClusterType:        clusters.TypeGreenplumCluster,
		Environment:        args.Environment,
		NetworkID:          args.NetworkID,
		FolderID:           session.FolderCoords.FolderID,
		Description:        args.Description,
		Labels:             args.Labels,
		DeletionProtection: args.DeletionProtection,
		HostGroupIDs:       args.HostGroupIDs,
		MaintenanceWindow:  args.MaintenanceWindow,
	})
	if err != nil {
		return clusters.Cluster{}, nil, nil, err
	}

	// Create master subcluster
	scm, err := creator.CreateSubCluster(ctx, models.CreateSubClusterArgs{
		ClusterID: c.ClusterID,
		Name:      masterSubclusterName,
		Roles:     []hosts.Role{hosts.RoleGreenplumMasterNode},
		Revision:  c.Revision,
	})
	if err != nil {
		return clusters.Cluster{}, nil, nil, err
	}

	// Create segment subcluster
	scs, err := creator.CreateSubCluster(ctx, models.CreateSubClusterArgs{
		ClusterID: c.ClusterID,
		Name:      segmentSubclusterName,
		Roles:     []hosts.Role{hosts.RoleGreenplumSegmentNode},
		Revision:  c.Revision,
	})
	if err != nil {
		return clusters.Cluster{}, nil, nil, err
	}

	masterSegment := gppillars.SegmentData{}

	// Create Master hosts
	masterHosts := make([]string, 0, args.MasterHostCount)
	for i := 0; i < args.MasterHostCount; i++ {
		fqdn, err := creator.GenerateFQDN(args.Config.ZoneID, resolvedMasterHostGroup.TargetResourcePreset().VType, compute.Ubuntu)
		if err != nil {
			return clusters.Cluster{}, nil, nil, err
		}
		masterHosts = append(masterHosts, fqdn)
	}
	sort.Strings(masterHosts)
	masterSegment.Primary.Fqdn = masterHosts[0]
	masterSegment.Primary.MountPoint = defaultMountPoint
	if len(masterHosts) > 1 {
		masterSegment.Mirror = &gppillars.SegmentPlace{Fqdn: masterHosts[1], MountPoint: defaultMountPoint}
	}
	for _, fqdn := range masterHosts {
		subnet, err := gp.compute.PickSubnet(ctx, subnets, resolvedMasterHostGroup.TargetResourcePreset().VType, args.Config.ZoneID,
			args.Config.AssignPublicIP, optional.String{String: args.Config.SubnetID, Valid: args.Config.SubnetID != ""}, session.FolderCoords.FolderExtID)
		if err != nil {
			return clusters.Cluster{}, nil, nil, err
		}
		_, err = creator.AddHosts(ctx, []models.AddHostArgs{{
			SubClusterID:     scm.SubClusterID,
			ResourcePresetID: resolvedMasterHostGroup.TargetResourcePreset().ID,
			SpaceLimit:       args.MasterConfig.Resources.DiskSize,
			ZoneID:           args.Config.ZoneID,
			FQDN:             fqdn,
			DiskTypeExtID:    args.MasterConfig.Resources.DiskTypeExtID,
			SubnetID:         subnet.ID,
			AssignPublicIP:   args.Config.AssignPublicIP,
			ClusterID:        c.ClusterID,
			Revision:         c.Revision,
		}})
		if err != nil {
			return clusters.Cluster{}, nil, nil, err
		}
		if gp.cfg.EnvironmentVType == environment.VTypeCompute {
			_, err = creator.AddPlacementGroup(ctx, models.AddPlacementGroupArgs{
				ClusterID: c.ClusterID,
				LocalID:   -1,
				FQDN:      fqdn,
				Revision:  c.Revision,
			})
			if err != nil {
				return clusters.Cluster{}, nil, nil, err
			}
		}
	}

	segments := map[int]gppillars.SegmentData{-1: masterSegment}

	localIDAdd := 0
	if args.MasterConfig.Resources.DiskTypeExtID == resources.NetworkSSDNonreplicated {
		localIDAdd++
		_, err = creator.AddDiskPlacementGroup(ctx, models.AddDiskPlacementGroupArgs{
			ClusterID: c.ClusterID,
			LocalID:   0,
			Revision:  c.Revision,
		})
		if err != nil {
			return clusters.Cluster{}, nil, nil, err
		}

		_, err = creator.AddDisk(ctx, models.AddDiskArgs{
			ClusterID:  c.ClusterID,
			LocalID:    0,
			FQDN:       masterSegment.Primary.Fqdn,
			MountPoint: masterSegment.Primary.MountPoint,
			Revision:   c.Revision,
		})
		if err != nil {
			return clusters.Cluster{}, nil, nil, err
		}

		if masterSegment.Mirror != nil {
			_, err = creator.AddDisk(ctx, models.AddDiskArgs{
				ClusterID:  c.ClusterID,
				LocalID:    0,
				FQDN:       masterSegment.Mirror.Fqdn,
				MountPoint: masterSegment.Mirror.MountPoint,
				Revision:   c.Revision,
			})
			if err != nil {
				return clusters.Cluster{}, nil, nil, err
			}
		}
	}

	segmentHosts := make([]string, 0, args.SegmentHostCount)
	// Create Segment hosts
	for i := 0; i < args.SegmentHostCount; i++ {
		fqdn, err := creator.GenerateFQDN(args.Config.ZoneID, resolvedSegmentHostGroup.TargetResourcePreset().VType, compute.Ubuntu)
		if err != nil {
			return clusters.Cluster{}, nil, nil, err
		}
		segmentHosts = append(segmentHosts, fqdn)
		subnet, err := gp.compute.PickSubnet(ctx, subnets, resolvedSegmentHostGroup.TargetResourcePreset().VType, args.Config.ZoneID,
			false, optional.String{String: args.Config.SubnetID, Valid: args.Config.SubnetID != ""}, session.FolderCoords.FolderExtID)
		if err != nil {
			return clusters.Cluster{}, nil, nil, err
		}
		_, err = creator.AddHosts(ctx, []models.AddHostArgs{{
			SubClusterID:     scs.SubClusterID,
			ResourcePresetID: resolvedSegmentHostGroup.TargetResourcePreset().ID,
			SpaceLimit:       args.SegmentConfig.Resources.DiskSize,
			ZoneID:           args.Config.ZoneID,
			FQDN:             fqdn,
			DiskTypeExtID:    args.SegmentConfig.Resources.DiskTypeExtID,
			SubnetID:         subnet.ID,
			AssignPublicIP:   false,
			ClusterID:        c.ClusterID,
			Revision:         c.Revision,
		}})
		if err != nil {
			return clusters.Cluster{}, nil, nil, err
		}
	}

	groupSize := placementGroupMaxSize
	if args.SegmentConfig.Resources.DiskTypeExtID != resources.NetworkSSDNonreplicated {
		groupSize = 2
	}

	var hds map[int][]hostDisk
	_, hds, err = generateSegments(segments, 0, segmentHosts, args.SegmentInHost, args.Config.SegmentMirroringEnable, groupSize)
	if err != nil {
		return clusters.Cluster{}, nil, nil, err
	}

	for LocalID, hd := range hds {
		if gp.cfg.EnvironmentVType == environment.VTypeCompute {
			for _, hostDisk := range hd {
				_, err = creator.AddPlacementGroup(ctx, models.AddPlacementGroupArgs{
					ClusterID: c.ClusterID,
					LocalID:   LocalID,
					FQDN:      hostDisk.fqdn,
					Revision:  c.Revision,
				})
				if err != nil {
					return clusters.Cluster{}, nil, nil, err
				}
			}
		}
	}

	if args.SegmentConfig.Resources.DiskTypeExtID == resources.NetworkSSDNonreplicated {
		for localID, hd := range hds {
			_, err = creator.AddDiskPlacementGroup(ctx, models.AddDiskPlacementGroupArgs{
				ClusterID: c.ClusterID,
				LocalID:   localID + localIDAdd,
				Revision:  c.Revision,
			})
			if err != nil {
				return clusters.Cluster{}, nil, nil, err
			}

			for _, hostDisk := range hd {
				_, err = creator.AddDisk(ctx, models.AddDiskArgs{
					ClusterID:  c.ClusterID,
					LocalID:    localID + localIDAdd,
					FQDN:       hostDisk.fqdn,
					MountPoint: hostDisk.mountPoint,
					Revision:   c.Revision,
				})
				if err != nil {
					return clusters.Cluster{}, nil, nil, err
				}
			}
		}
	}

	// validate security groups after NetworkID validation
	if len(args.SecurityGroupIDs) > 0 {
		err = gp.compute.ValidateSecurityGroups(ctx, args.SecurityGroupIDs, args.NetworkID)
		if err != nil {
			return clusters.Cluster{}, nil, nil, err
		}
	}

	err = creator.SetDefaultVersionCluster(ctx, c.ClusterID,
		clusters.TypeGreenplumCluster, args.Environment, ver.MajorVersion, ver.Edition, c.Revision)

	if err != nil {
		return clusters.Cluster{}, nil, nil, err
	}

	// add backup schedule
	backupSchedule := bmodels.GetBackupSchedule(gp.cfg.Greenplum.BackupSchedule, args.Config.BackupWindowStart)
	if err := gp.backups.AddBackupSchedule(ctx, c.ClusterID, backupSchedule, c.Revision); err != nil {
		return clusters.Cluster{}, nil, nil, err
	}

	return c, privKey, segments, nil

}

func (gp *Greenplum) fillUsersInPillar(
	pillar *gppillars.Cluster,
	adminUserName string) error {

	err := pillar.GenerateGPAdminUser(gp.cryptoProvider)
	if err != nil {
		return err
	}

	err = pillar.GenerateMonitorUser(gp.cryptoProvider)
	if err != nil {
		return err
	}

	pillar.Data.Greenplum.AdminUserName = adminUserName
	return nil
}

func (gp *Greenplum) makeClusterPillar(c clusters.Cluster, privKey []byte, args greenplum.CreateClusterArgs, segments map[int]gppillars.SegmentData) (*gppillars.Cluster, error) {
	pillar := gppillars.NewCluster()
	privateKeyEnc, err := gp.cryptoProvider.Encrypt(privKey)
	if err != nil {
		return nil, err
	}
	pillar.Data.ClusterPrivateKey = privateKeyEnc

	if gp.cfg.E2E.IsClusterE2E(c.Name, args.FolderExtID) {
		pillar.Data.Billing = pillars.NewDisabledBilling()
		pillar.Data.ShipLogs = new(bool)
		pillar.Data.MDBHealth = pillars.NewMDBHealthWithDisabledAggregate()
	}

	pillar.Data.S3Bucket = gp.cfg.S3BucketName(c.ClusterID)
	pillar.Data.Greenplum.Config = args.Config

	if args.ConfigSpec.Config.MaxConnections.Valid {
		pillar.Data.Greenplum.Config.MaxConnections.Master = &args.ConfigSpec.Config.MaxConnections.Int64
	}
	if args.ConfigSpec.Config.GPWorkfileLimitFilesPerQuery.Valid {
		pillar.Data.Greenplum.Config.GPWorkfileLimitFilesPerQuery = &args.ConfigSpec.Config.GPWorkfileLimitFilesPerQuery.Int64
	}
	if args.ConfigSpec.Config.MaxPreparedTransactions.Valid {
		pillar.Data.Greenplum.Config.MaxPreparedTransactions = &args.ConfigSpec.Config.MaxPreparedTransactions.Int64
	}
	if args.ConfigSpec.Config.GPWorkfileLimitPerQuery.Valid {
		pillar.Data.Greenplum.Config.GPWorkfileLimitPerQuery = args.ConfigSpec.Config.ToPillar().GPWorkfileLimitPerQuery
	}

	if args.ConfigSpec.Config.GPWorkfileLimitPerSegment.Valid {
		pillar.Data.Greenplum.Config.GPWorkfileLimitPerSegment = args.ConfigSpec.Config.ToPillar().GPWorkfileLimitPerSegment
	}
	if args.ConfigSpec.Config.MaxSlotWalKeepSize.Valid {
		pillar.Data.Greenplum.Config.MaxSlotWalKeepSize = args.ConfigSpec.Config.ToPillar().MaxSlotWalKeepSize
	}
	if args.ConfigSpec.Config.GPWorkfileCompression.Valid {
		pillar.Data.Greenplum.Config.GPWorkfileCompression = &args.ConfigSpec.Config.GPWorkfileCompression.Bool
	}
	if args.ConfigSpec.Config.MaxStatementMem.Valid {
		pillar.Data.Greenplum.Config.MaxStatementMem = args.ConfigSpec.Config.ToPillar().MaxStatementMem
	}
	if args.ConfigSpec.Config.LogStatement != gpmodels.LogStatementUnspecified {
		pillar.Data.Greenplum.Config.LogStatement = args.ConfigSpec.Config.ToPillar().LogStatement
	}

	pillar.Data.Pool = args.ConfigSpec.Pool.ToPillar()
	pillar.Data.Greenplum.MasterHostCount = int64(args.MasterHostCount)
	pillar.Data.Greenplum.SegmentHostCount = int64(args.SegmentHostCount)
	pillar.Data.Greenplum.SegmentInHost = int64(args.SegmentInHost)
	pillar.Data.Greenplum.Segments = segments

	if args.Config.Access != nil {
		pillar.Data.Access.WebSQL = args.Config.Access.WebSQL
		pillar.Data.Access.DataLens = args.Config.Access.DataLens
		pillar.Data.Access.DataTransfer = args.Config.Access.DataTransfer
		pillar.Data.Access.Serverless = args.Config.Access.Serverless
	}

	return pillar, nil
}

func (gp *Greenplum) DeleteCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return gp.operator.Delete(ctx, cid, clusters.TypeGreenplumCluster,
		func(ctx context.Context, session sessions.Session, cluster clusterslogic.Cluster, _ clusterslogic.Reader) (operations.Operation, error) {
			var pillar gppillars.Cluster
			err := cluster.Pillar(&pillar)
			if err != nil {
				return operations.Operation{}, err
			}

			var s3Buckets = taskslogic.DeleteClusterS3Buckets{
				"backup": pillar.Data.S3Bucket,
			}

			op, err := gp.tasks.DeleteCluster(
				ctx,
				session,
				cluster.ClusterID,
				cluster.Revision,
				taskslogic.DeleteClusterTaskTypes{
					Delete:   gpmodels.TaskTypeClusterDelete,
					Metadata: gpmodels.TaskTypeClusterDeleteMetadata,
					Purge:    gpmodels.TaskTypeClusterPurge,
				},
				gpmodels.OperationTypeClusterDelete,
				s3Buckets,
			)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := gp.search.StoreDocDelete(ctx, greenplumService, session.FolderCoords.FolderExtID, session.FolderCoords.CloudExtID, op); err != nil {
				return operations.Operation{}, err
			}
			return op, nil
		},
	)
}

func (gp *Greenplum) StartCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return gp.operator.ModifyOnNotRunningCluster(ctx, cid, clusters.TypeGreenplumCluster,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, _ clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			op, err := gp.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      gpmodels.TaskTypeClusterStart,
					OperationType: gpmodels.OperationTypeClusterStart,
					Revision:      cluster.Revision,
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}
			return op, nil
		},
	)
}
func (gp *Greenplum) StopCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return gp.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeGreenplumCluster,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, _ clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			op, err := gp.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      gpmodels.TaskTypeClusterStop,
					OperationType: gpmodels.OperationTypeClusterStop,
					Revision:      cluster.Revision,
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}
			return op, nil
		},
	)
}

func (gp *Greenplum) ListMasterHosts(ctx context.Context, cid string) ([]hosts.HostExtended, error) {
	// TODO: pagination
	hostsList, _, _, err := clusterslogic.ListHosts(ctx, gp.operator, cid, clusters.TypeGreenplumCluster, -1, 0)

	if err != nil {
		return nil, err
	}

	res := make([]hosts.HostExtended, 0, len(hostsList))

	for _, host := range hostsList {
		if host.Roles[0] == hosts.RoleGreenplumMasterNode {
			res = append(res, host)
		}
	}

	return res, err
}
func (gp *Greenplum) ListSegmentHosts(ctx context.Context, cid string) ([]hosts.HostExtended, error) {
	// TODO: pagination
	hostsList, _, _, err := clusterslogic.ListHosts(ctx, gp.operator, cid, clusters.TypeGreenplumCluster, -1, 0)

	if err != nil {
		return nil, err
	}

	res := make([]hosts.HostExtended, 0, len(hostsList))

	for _, host := range hostsList {
		if host.Roles[0] == hosts.RoleGreenplumSegmentNode {
			res = append(res, host)
		}
	}

	return res, err
}

func (gp *Greenplum) EstimateCreateCluster(ctx context.Context, args greenplum.CreateClusterArgs) (console.BillingEstimate, error) {
	versions, err := gp.console.GetDefaultVersions(ctx, clusters.TypeGreenplumCluster, args.Environment, componentName)

	if err != nil {
		return console.BillingEstimate{}, err
	}

	if _, err = args.GetValidatedVersion(versions, true); err != nil {
		return console.BillingEstimate{}, err
	}

	var billingEstimate console.BillingEstimate

	err = gp.operator.ReadOnFolder(ctx, args.FolderExtID,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			hostBillingSpecs := make([]clusterslogic.HostBillingSpec, 0, args.SegmentHostCount+args.MasterHostCount)
			for i := 0; i < args.MasterHostCount; i++ {
				hostBillingSpecs = append(hostBillingSpecs, clusterslogic.HostBillingSpec{
					HostRole:         hosts.RoleGreenplumMasterNode,
					ClusterResources: args.MasterConfig.Resources,
					AssignPublicIP:   args.Config.AssignPublicIP,
					OnDedicatedHost:  len(args.HostGroupIDs) > 0,
				})
			}
			for i := 0; i < args.SegmentHostCount; i++ {
				hostBillingSpecs = append(hostBillingSpecs, clusterslogic.HostBillingSpec{
					HostRole:         hosts.RoleGreenplumSegmentNode,
					ClusterResources: args.SegmentConfig.Resources,
					AssignPublicIP:   false,
					OnDedicatedHost:  len(args.HostGroupIDs) > 0,
				})
			}

			var err error
			billingEstimate, err = reader.EstimateBilling(ctx, args.FolderExtID, clusters.TypeGreenplumCluster, hostBillingSpecs, environment.CloudTypeYandex)
			return err
		},
	)
	return billingEstimate, err
}

func (gp *Greenplum) GetDefaultVersions(ctx context.Context) ([]console.DefaultVersion, error) {
	versions, err := gp.console.GetDefaultVersions(ctx, clusters.TypeGreenplumCluster, gp.cfg.SaltEnvs.Production, componentName)

	return versions, err
}

func (gp *Greenplum) GetHostGroupType(ctx context.Context, hostGroupIds []string) (map[string]compute2.HostGroupHostType, error) {

	return gp.compute.GetHostGroupHostType(ctx, hostGroupIds)

}

func (gp *Greenplum) toSearchQueue(ctx context.Context, folderCoords metadb.FolderCoords, op operations.Operation) error {
	return gp.search.StoreDoc(
		ctx,
		greenplumService,
		folderCoords.FolderExtID,
		folderCoords.CloudExtID,
		op,
		searchAttributesExtractor,
	)
}

func (gp *Greenplum) ModifyCluster(ctx context.Context, args greenplum.ModifyClusterArgs) (operations.Operation, error) {
	return gp.operator.ModifyOnCluster(ctx, args.ClusterID, clusters.TypeGreenplumCluster,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			clusterChanges := common.GetClusterChanges()
			clusterChanges.Timeout = 15 * time.Minute
			var changedSecurityGroupIDs optional.Strings
			var err error
			var pillarChanges, cfgChanges, needRestart, passwordChanges bool

			clusterChanges.HasMetadataChanges, err = modifier.ModifyClusterMetadata(ctx, cluster, args.Name, args.Labels)
			if err != nil {
				return operations.Operation{}, err
			}

			/* modify metadb-only parameters */
			clusterChanges.HasMetaDBChanges, err = modifier.ModifyClusterMetadataParameters(ctx, cluster, args.Description, args.Labels, args.DeletionProtection, args.MaintenanceWindow)
			if err != nil {
				return operations.Operation{}, err
			}
			err = args.ConfigSpec.Config.Validate()
			if err != nil {
				return operations.Operation{}, err
			}
			pillar := gppillars.NewCluster()
			pillar.DisallowUnknownFields()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			if args.UserPassword.Valid {
				err := pillar.UpdateUserPassword(args.UserPassword.String, gp.cryptoProvider)
				if err != nil {
					return operations.Operation{}, err
				}
				pillarChanges = true
				passwordChanges = true
			}

			if args.Access.DataLens.Valid && args.Access.DataLens.Bool != pillar.Data.Access.DataLens {
				pillar.Data.Access.DataLens = args.Access.DataLens.Bool
				pillarChanges = true
			}
			if args.Access.WebSQL.Valid && args.Access.WebSQL.Bool != pillar.Data.Access.WebSQL {
				pillar.Data.Access.WebSQL = args.Access.WebSQL.Bool
				pillarChanges = true
			}
			if args.Access.DataTransfer.Valid && args.Access.DataTransfer.Bool != pillar.Data.Access.DataTransfer {
				pillar.Data.Access.DataTransfer = args.Access.DataTransfer.Bool
				pillarChanges = true
			}
			if args.Access.Serverless.Valid && args.Access.Serverless.Bool != pillar.Data.Access.Serverless {
				pillar.Data.Access.Serverless = args.Access.Serverless.Bool
				pillarChanges = true
			}

			if args.ConfigSpec.Pool.PoolSize.Valid && &args.ConfigSpec.Pool.PoolSize.Int64 != pillar.Data.Pool.PoolSize {
				pillar.Data.Pool.PoolSize = &args.ConfigSpec.Pool.PoolSize.Int64
				cfgChanges = true
			}

			if args.ConfigSpec.Pool.Mode.Valid && &args.ConfigSpec.Pool.Mode.String != pillar.Data.Pool.Mode {
				pillar.Data.Pool.Mode = &args.ConfigSpec.Pool.Mode.String
				cfgChanges = true
			}

			if args.ConfigSpec.Pool.ClientIdleTimeout.Valid && &args.ConfigSpec.Pool.ClientIdleTimeout.Int64 != pillar.Data.Pool.ClientIdleTimeout {
				pillar.Data.Pool.ClientIdleTimeout = &args.ConfigSpec.Pool.ClientIdleTimeout.Int64
				cfgChanges = true
			}

			if args.ConfigSpec.Config.MaxConnections.Valid && &args.ConfigSpec.Config.MaxConnections.Int64 != pillar.Data.Greenplum.Config.MaxConnections.Master {
				pillar.Data.Greenplum.Config.MaxConnections.Master = &args.ConfigSpec.Config.MaxConnections.Int64
				cfgChanges = true
			}

			if args.ConfigSpec.Config.MaxPreparedTransactions.Valid && &args.ConfigSpec.Config.MaxPreparedTransactions.Int64 != pillar.Data.Greenplum.Config.MaxPreparedTransactions {
				pillar.Data.Greenplum.Config.MaxPreparedTransactions = &args.ConfigSpec.Config.MaxPreparedTransactions.Int64
				cfgChanges = true
			}

			if args.ConfigSpec.Config.GPWorkfileCompression.Valid && &args.ConfigSpec.Config.GPWorkfileCompression.Bool != pillar.Data.Greenplum.Config.GPWorkfileCompression {
				pillar.Data.Greenplum.Config.GPWorkfileCompression = &args.ConfigSpec.Config.GPWorkfileCompression.Bool
				cfgChanges = true
			}

			if args.ConfigSpec.Config.MaxSlotWalKeepSize.Valid && args.ConfigSpec.Config.ToPillar().MaxSlotWalKeepSize != pillar.Data.Greenplum.Config.MaxSlotWalKeepSize {
				pillar.Data.Greenplum.Config.MaxSlotWalKeepSize = args.ConfigSpec.Config.ToPillar().MaxSlotWalKeepSize
				cfgChanges = true
			}

			if args.ConfigSpec.Config.GPWorkfileLimitPerSegment.Valid && args.ConfigSpec.Config.ToPillar().GPWorkfileLimitPerSegment != pillar.Data.Greenplum.Config.GPWorkfileLimitPerSegment {
				pillar.Data.Greenplum.Config.GPWorkfileLimitPerSegment = args.ConfigSpec.Config.ToPillar().GPWorkfileLimitPerSegment
				cfgChanges = true
			}

			if args.ConfigSpec.Config.GPWorkfileLimitPerQuery.Valid && args.ConfigSpec.Config.ToPillar().GPWorkfileLimitPerQuery != pillar.Data.Greenplum.Config.GPWorkfileLimitPerQuery {
				pillar.Data.Greenplum.Config.GPWorkfileLimitPerQuery = args.ConfigSpec.Config.ToPillar().GPWorkfileLimitPerQuery
				cfgChanges = true
			}

			if args.ConfigSpec.Config.GPWorkfileLimitFilesPerQuery.Valid && &args.ConfigSpec.Config.GPWorkfileLimitFilesPerQuery.Int64 != pillar.Data.Greenplum.Config.GPWorkfileLimitFilesPerQuery {
				pillar.Data.Greenplum.Config.GPWorkfileLimitFilesPerQuery = &args.ConfigSpec.Config.GPWorkfileLimitFilesPerQuery.Int64
				cfgChanges = true
			}
			if args.ConfigSpec.Config.MaxStatementMem.Valid &&
				(pillar.Data.Greenplum.Config.MaxStatementMem == nil ||
					pillar.Data.Greenplum.Config.MaxStatementMem != nil &&
						*args.ConfigSpec.Config.ToPillar().MaxStatementMem != *pillar.Data.Greenplum.Config.MaxStatementMem) {
				pillar.Data.Greenplum.Config.MaxStatementMem = args.ConfigSpec.Config.ToPillar().MaxStatementMem
				cfgChanges = true
			}
			if args.ConfigSpec.Config.LogStatement != gpmodels.LogStatementUnspecified &&
				(pillar.Data.Greenplum.Config.LogStatement.Master == nil ||
					pillar.Data.Greenplum.Config.LogStatement.Master != nil &&
						*args.ConfigSpec.Config.ToPillar().LogStatement.Master != *pillar.Data.Greenplum.Config.LogStatement.Master) {
				pillar.Data.Greenplum.Config.LogStatement = args.ConfigSpec.Config.ToPillar().LogStatement
				cfgChanges = true
			}

			resChanges, resTimeout, err := gp.modifyResources(ctx, reader, modifier, cluster.Cluster, args, session)
			if err != nil {
				return operations.Operation{}, err
			}
			clusterChanges.Timeout += resTimeout
			clusterChanges.HasChanges = clusterChanges.HasChanges || resChanges
			needRestart = needRestart || resChanges

			if args.SecurityGroupsIDs.Valid && !slices.EqualAnyOrderStrings(args.SecurityGroupsIDs.Strings, cluster.SecurityGroupIDs) {
				changedSecurityGroupIDs.Set(slices.DedupStrings(args.SecurityGroupsIDs.Strings))
				if err := gp.compute.ValidateSecurityGroups(ctx, changedSecurityGroupIDs.Strings, cluster.NetworkID); err != nil {
					return operations.Operation{}, err
				}
				clusterChanges.HasChanges = true
				needRestart = true
			}

			if pillarChanges || cfgChanges {
				err = modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar)
				if err != nil {
					return operations.Operation{}, xerrors.Errorf("failed to update pillar: %w", err)
				}
				clusterChanges.HasChanges = true
			}

			if args.BackupWindowStart.Valid && !reflect.DeepEqual(args.BackupWindowStart.Value, cluster.BackupSchedule.Start) {
				backupSchedule := bmodels.GetBackupSchedule(cluster.BackupSchedule, args.BackupWindowStart)
				err := gp.backups.AddBackupSchedule(ctx, cluster.ClusterID, backupSchedule, cluster.Revision)
				if err != nil {
					return operations.Operation{}, xerrors.Errorf("failed to update backup schedule: %w", err)
				}
				clusterChanges.HasChanges = true
			}

			clusterChanges.TaskArgs = map[string]interface{}{
				"restart":        needRestart || cfgChanges,
				"sync-passwords": passwordChanges,
			}

			info := getTaskCreationInfo()
			op, err := common.CreateClusterModifyOperation(gp.tasks, ctx, session, cluster.Cluster, searchAttributesExtractor, clusterChanges, changedSecurityGroupIDs, info)
			if err != nil {
				return operations.Operation{}, err
			}

			if op.Type == info.MetadataUpdateOperation {
				if err := gp.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
					return operations.Operation{}, err
				}

				return op, nil
			}

			return op, nil
		},
	)
}

func (gp *Greenplum) RescheduleMaintenance(ctx context.Context, cid string, rescheduleType clusters.RescheduleType, delayedUntil optional.Time) (operations.Operation, error) {
	return gp.operator.ModifyOnClusterWithoutRevChanging(ctx, cid, clusters.TypeGreenplumCluster,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			newMaintenanceTime, err := modifier.RescheduleMaintenance(ctx, cluster.ClusterID, rescheduleType, delayedUntil)
			if err != nil {
				return operations.Operation{}, err
			}

			op, err := gp.tasks.CreateFinishedTask(
				ctx,
				session,
				cluster.ClusterID,
				cluster.Revision,
				gpmodels.OperationTypeRescheduleMaintenance,
				gpmodels.RescheduleMaintenanceMetadata{
					DelayedUntil: timestamppb.New(newMaintenanceTime),
				},
				true,
			)
			if err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
	)
}

func (gp *Greenplum) modifyResources(ctx context.Context, reader clusterslogic.Reader, modifier clusterslogic.Modifier,
	cluster clusters.Cluster, args greenplum.ModifyClusterArgs, session sessions.Session) (bool, time.Duration, error) {
	var hasChanges bool
	var timeout time.Duration

	if args.MasterConfig.Resource.DiskSize.IsSet || args.MasterConfig.Resource.ResourcePresetExtID.IsSet {
		changes, extraTimeout, err := gp.modifyHostByRole(
			ctx, reader, modifier, cluster, hosts.RoleGreenplumMasterNode,
			args.MasterConfig.Resource.ResourcePresetExtID,
			args.MasterConfig.Resource.DiskSize,
			session)

		if err != nil {
			return false, 0, err
		}
		hasChanges = hasChanges || changes
		timeout += extraTimeout
	}

	if args.SegmentConfig.Resource.DiskSize.IsSet || args.SegmentConfig.Resource.ResourcePresetExtID.IsSet {
		changes, extraTimeout, err := gp.modifyHostByRole(
			ctx, reader, modifier, cluster, hosts.RoleGreenplumSegmentNode,
			args.SegmentConfig.Resource.ResourcePresetExtID,
			args.SegmentConfig.Resource.DiskSize,
			session)

		if err != nil {
			return false, 0, err
		}
		hasChanges = hasChanges || changes
		timeout += extraTimeout
	}

	return hasChanges, timeout, nil
}

func (gp *Greenplum) modifyHostByRole(ctx context.Context, reader clusterslogic.Reader, modifier clusterslogic.Modifier,
	cluster clusters.Cluster, role hosts.Role,
	reqResourcePresetExtID greenplum.OptString,
	reqDiskSize greenplum.OptInt,
	session sessions.Session) (bool, time.Duration, error) {
	var hasChanges bool
	var timeout time.Duration

	currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cluster.ClusterID)
	if err != nil {
		return false, 0, err
	}

	hostsWithRole := clusterslogic.GetHostsWithRole(currentHosts, role)
	if len(hostsWithRole) == 0 {
		return false, 0, xerrors.Errorf("failed to find host with role %q within cluster %q", role.String(), cluster.ClusterID)
	}

	anyHost := hostsWithRole[0]

	if role == hosts.RoleGreenplumMasterNode && len(hostsWithRole) != 2 {
		return false, 0, semerr.InvalidInputf("need 2 master hosts for cluster %q", cluster.ClusterID)
	}

	diskSize := anyHost.SpaceLimit
	if reqDiskSize.IsSet {
		isLocalDisk := resources.DiskTypes{}.IsLocalDisk(anyHost.DiskTypeExtID)
		vType, err := environment.ParseVType(anyHost.VType)
		if err != nil {
			return false, 0, err
		}
		if *reqDiskSize.Value < anyHost.SpaceLimit && isLocalDisk && vType == environment.VTypeCompute {
			return false, 0, semerr.InvalidInput("local disk cannot be sized down")
		}

		diskSize = *reqDiskSize.Value
	}

	resourcePresetExtID := anyHost.ResourcePresetExtID
	if reqResourcePresetExtID.IsSet {
		resourcePresetExtID = *reqResourcePresetExtID.Value
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

	hostGroup := clusterslogic.HostGroup{
		Role:                       role,
		CurrentResourcePresetExtID: optional.NewString(anyHost.ResourcePresetExtID),
		NewResourcePresetExtID:     optional.NewString(resourcePresetExtID),
		DiskTypeExtID:              anyHost.DiskTypeExtID,
		CurrentDiskSize:            optional.NewInt64(anyHost.SpaceLimit),
		NewDiskSize:                optional.NewInt64(diskSize),
		HostsCurrent:               zoneHostsList,
	}

	resolvedHostGroups, hasChanges, err := modifier.ValidateResources(
		ctx,
		session,
		clusters.TypeGreenplumCluster,
		hostGroup,
	)
	if err != nil {
		return false, 0, err
	}

	if !hasChanges {
		return false, 0, nil
	}

	resolvedHostGroup := resolvedHostGroups.Single()

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

		// expect 1 GB to be transferred in 60 seconds + 5 minute per host
		timeout += time.Duration(host.SpaceLimit*60/(1<<30))*time.Second + 5*time.Minute
	}

	return hasChanges, timeout, nil
}

func (gp *Greenplum) AddHosts(ctx context.Context, args greenplum.AddHostsClusterArgs) (operations.Operation, error) {
	return gp.operator.ModifyOnCluster(ctx, args.ClusterID, clusters.TypeGreenplumCluster,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			var changedSecurityGroupIDs optional.Strings
			timeout := 60 * time.Minute
			var hasChanges bool

			taskArgs := map[string]interface{}{
				"restart": false,
			}

			pillar := gppillars.NewCluster()
			pillar.DisallowUnknownFields()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			if args.MasterHostCount == 1 && pillar.Data.Greenplum.MasterHostCount == 2 {
				return operations.Operation{}, semerr.InvalidInput("only 2 master hosts are allowed.")
			} else if args.MasterHostCount > 1 {
				return operations.Operation{}, semerr.InvalidInput("you can add only one master host")
			}

			if args.SegmentHostCount == 1 {
				return operations.Operation{}, semerr.InvalidInput("you can add only two or more hosts")
			}

			if args.MasterHostCount == 1 {
				change, d, fqdn, err := gp.addHostByRole(ctx, reader, modifier,
					cluster.Cluster, hosts.RoleGreenplumMasterNode,
					pillar,
					session)

				if err != nil {
					return operations.Operation{}, err
				}
				timeout += d
				hasChanges = hasChanges || change

				seg := pillar.Data.Greenplum.Segments[-1]
				seg.Mirror = &gppillars.SegmentPlace{
					Fqdn:       fqdn,
					MountPoint: defaultMountPoint,
				}
				pillar.Data.Greenplum.Segments[-1] = seg
				pillar.Data.Greenplum.MasterHostCount++

				taskArgs["hosts_create_master"] = fqdn
			}

			var segmentHosts []string
			for i := 0; i < int(args.SegmentHostCount); i++ {
				change, d, fqdn, err := gp.addHostByRole(ctx, reader, modifier,
					cluster.Cluster, hosts.RoleGreenplumSegmentNode,
					pillar,
					session)

				if err != nil {
					return operations.Operation{}, err
				}
				timeout += d
				hasChanges = hasChanges || change

				segmentHosts = append(segmentHosts, fqdn)
			}

			if args.SegmentHostCount > 0 {
				groupSize := placementGroupMaxSize

				firstAddedSegment := pillar.Data.Greenplum.SegmentHostCount * pillar.Data.Greenplum.SegmentInHost

				_, _, err := generateSegments(pillar.Data.Greenplum.Segments,
					int(firstAddedSegment),
					segmentHosts, int(pillar.Data.Greenplum.SegmentInHost), pillar.Data.Greenplum.Config.SegmentMirroringEnable,
					groupSize)
				if err != nil {
					return operations.Operation{}, err
				}
				pillar.Data.Greenplum.SegmentHostCount += args.SegmentHostCount

				taskArgs["hosts_create_segments"] = segmentHosts
				taskArgs["first_added_segment"] = firstAddedSegment
				taskArgs["add_segments_count"] = args.SegmentHostCount * pillar.Data.Greenplum.SegmentInHost
			}

			if hasChanges {
				err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar)
				if err != nil {
					return operations.Operation{}, xerrors.Errorf("failed to update pillar: %w", err)
				}

				return gp.tasks.ModifyCluster(
					ctx,
					session,
					cluster.ClusterID,
					cluster.Revision,
					gpmodels.TaskTypeHostCreate,
					gpmodels.OperationTypeHostCreate,
					changedSecurityGroupIDs,
					greenplumService,
					searchAttributesExtractor,
					taskslogic.ModifyTimeout(timeout),
					taskslogic.ModifyTaskArgs(taskArgs),
				)
			}

			return operations.Operation{}, semerr.InvalidInput("no changes detected")
		},
	)
}

func (gp *Greenplum) addHostByRole(ctx context.Context, reader clusterslogic.Reader, modifier clusterslogic.Modifier,
	cluster clusters.Cluster, role hosts.Role,
	pillar *gppillars.Cluster,
	session sessions.Session,
) (bool, time.Duration, string, error) {
	var timeout time.Duration

	currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cluster.ClusterID)
	if err != nil {
		return false, 0, "", err
	}

	hostsWithRole := clusterslogic.GetHostsWithRole(currentHosts, role)
	if len(hostsWithRole) == 0 {
		return false, 0, "", xerrors.Errorf("failed to find host with role %q within cluster %q for add another one", role.String(), cluster.ClusterID)
	}

	anyHost := hostsWithRole[0]

	diskSize := anyHost.SpaceLimit
	resourcePresetExtID := anyHost.ResourcePresetExtID

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

	hostGroup := clusterslogic.HostGroup{
		Role:                       role,
		CurrentResourcePresetExtID: optional.NewString(anyHost.ResourcePresetExtID),
		NewResourcePresetExtID:     optional.NewString(resourcePresetExtID),
		DiskTypeExtID:              anyHost.DiskTypeExtID,
		CurrentDiskSize:            optional.NewInt64(anyHost.SpaceLimit),
		NewDiskSize:                optional.NewInt64(diskSize),
		HostsCurrent:               zoneHostsList,
		HostsToAdd: []clusterslogic.ZoneHosts{{
			ZoneID: anyHost.ZoneID,
			Count:  1,
		}},
		SkipValidations: clusterslogic.SkipValidations{
			DiskSize: true,
		},
	}

	resolvedHostGroups, _, err := modifier.ValidateResources(
		ctx,
		session,
		clusters.TypeGreenplumCluster,
		hostGroup,
	)
	if err != nil {
		return false, 0, "", err
	}

	resolvedHostGroup := resolvedHostGroups.Single()

	fqdn, err := modifier.GenerateFQDN(pillar.Data.Greenplum.Config.ZoneID, resolvedHostGroup.TargetResourcePreset().VType, compute.Ubuntu)
	if err != nil {
		return false, 0, "", err
	}

	network, err := gp.compute.Network(ctx, cluster.NetworkID)
	if err != nil {
		return false, 0, "", err
	}
	var subnets []networkProvider.Subnet
	if network.ID != "" {
		subnets, err = gp.compute.Subnets(ctx, network)
		if err != nil {
			return false, 0, "", err
		}
	}

	subnet, err := gp.compute.PickSubnet(ctx, subnets, resolvedHostGroup.TargetResourcePreset().VType, anyHost.ZoneID, anyHost.AssignPublicIP,
		optional.NewString(anyHost.SubnetID), session.FolderCoords.FolderExtID)
	if err != nil {
		return false, 0, "", err
	}

	var hs []models.AddHostArgs
	hs = append(hs, models.AddHostArgs{
		ClusterID:        anyHost.ClusterID,
		SubClusterID:     anyHost.SubClusterID,
		ResourcePresetID: anyHost.ResourcePresetID,
		ZoneID:           anyHost.ZoneID,
		FQDN:             fqdn,
		DiskTypeExtID:    anyHost.DiskTypeExtID,
		SpaceLimit:       diskSize,
		SubnetID:         subnet.ID,
		AssignPublicIP:   anyHost.AssignPublicIP,
		Revision:         cluster.Revision,
	})

	_, err = modifier.AddHosts(ctx, hs)
	if err != nil {
		return false, 0, "", err
	}

	// expect 1 GB to be transferred in 60 seconds + 5 minute per host
	timeout += time.Duration(anyHost.SpaceLimit*60/(1<<30))*time.Second + 5*time.Minute

	return true, timeout, fqdn, nil
}

//TODO: when backup api will be enabled in all greenplum clusters, we should select backups from metadb using one query
// (here and in ./backups.go)
// til then we should check useBackupAPI per cid

func (gp *Greenplum) ClusterBackups(ctx context.Context, cid string, pageToken bmodels.BackupsPageToken, pageSize int64) ([]bmodels.Backup, bmodels.BackupsPageToken, error) {
	var backups []bmodels.Backup
	var nextPageToken bmodels.BackupsPageToken
	pageSize = pagination.SanePageSize(pageSize)

	var err error
	if err = gp.operator.ReadOnCluster(ctx, cid, clusters.TypeGreenplumCluster,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cluster clusterslogic.Cluster) error {
			backups, nextPageToken, err = gp.backups.BackupsByClusterID(ctx, cluster.ClusterID,
				func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
					return gp.listClusterBackups(ctx, reader, client, cid)
				},
				pageToken, optional.NewInt64(pageSize))

			for i := range backups {
				backups[i].FolderID = session.FolderCoords.FolderExtID
			}

			return err
		}); err != nil {
		return []bmodels.Backup{}, nextPageToken, err
	}

	return backups, nextPageToken, nil
}

func (gp *Greenplum) RestoreCluster(ctx context.Context, args greenplum.CreateClusterArgs, extBackupID string) (operations.Operation, error) {
	return gp.operator.Restore(ctx, args.FolderExtID,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, restorer clusterslogic.Restorer) (clusters.Cluster, operations.Operation, error) {
			// load source cluster info
			sourceCluster, sourceResourceMaster, sourceResourceSegment, backup, majorVersion, err := gp.loadSourceClusterInfo(ctx, reader, extBackupID)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}
			var sourcePillar gppillars.Cluster
			err = sourceCluster.Pillar(&sourcePillar)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			zoneID := args.Config.ZoneID
			subnetID := args.Config.SubnetID
			assignPublicIP := args.Config.AssignPublicIP
			access := args.Config.Access
			backupWindowStart := args.Config.BackupWindowStart

			args.Config = sourcePillar.Data.Greenplum.Config
			args.Config.ZoneID = zoneID
			args.Config.SubnetID = subnetID
			args.Config.AssignPublicIP = assignPublicIP
			args.Config.Access = access
			args.Config.BackupWindowStart = backupWindowStart
			args.Config.Version = majorVersion

			args.SecurityGroupIDs = slices.DedupStrings(args.SecurityGroupIDs)
			args.MasterConfig.Config = sourcePillar.Data.Greenplum.MasterConfig
			args.SegmentConfig.Config = sourcePillar.Data.Greenplum.SegmentConfig
			args.MasterHostCount = int(sourcePillar.Data.Greenplum.MasterHostCount)
			args.SegmentHostCount = int(sourcePillar.Data.Greenplum.SegmentHostCount)
			args.SegmentInHost = int(sourcePillar.Data.Greenplum.SegmentInHost)

			// validate restore possibility
			err = args.ValidateRestore(sourceResourceMaster, sourceResourceSegment)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			// Create cluster, subcluster and hosts
			cluster, privKey, segments, err := gp.createCluster(ctx, restorer, session, &args, true, true)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			// Save target pillar
			targetPillarID, err := restorer.AddTargetPillar(ctx, cluster.ClusterID, pillars.MakeTargetPillar(sourcePillar.Data))
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			// Create cluster pillar
			pillar, err := gp.makeClusterPillar(cluster, privKey, args, segments)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
			}

			pillar.Data.Greenplum.Users = sourcePillar.Data.Greenplum.Users
			err = gp.fillUsersInPillar(pillar, sourcePillar.Data.Greenplum.AdminUserName)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
			}

			err = restorer.AddClusterPillar(ctx, cluster.ClusterID, cluster.Revision, pillar)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
			}

			timeout, err := gp.calcClusterRestoreTimeout(ctx, args.SegmentConfig.Resources)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to calculate restore timeout: %w", err)
			}

			restoreOperation, err := gp.tasks.CreateCluster(
				ctx,
				session,
				cluster.ClusterID,
				cluster.Revision,
				gpmodels.TaskTypeClusterRestore,
				gpmodels.OperationTypeClusterRestore,
				gpmodels.MetadataRestoreCluster{
					SourceClusterID: sourceCluster.ClusterID,
					BackupID:        backup.ID,
				},
				optional.NewString(pillar.Data.S3Bucket),
				args.SecurityGroupIDs,
				greenplumService,
				searchAttributesExtractor,
				taskslogic.CreateTaskArgs(
					map[string]interface{}{
						"target-pillar-id": targetPillarID,
						"restore-from": map[string]interface{}{
							"cid":       sourceCluster.ClusterID,
							"backup-id": backup.ID,
							"s3-path":   pillar.Data.S3Bucket,
						},
					},
				),
				taskslogic.CreateTaskTimeout(timeout),
			)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			if err := gp.toSearchQueue(ctx, session.FolderCoords, restoreOperation); err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			return cluster, restoreOperation, nil
		},
	)
}

func (gp *Greenplum) calcClusterRestoreTimeout(ctx context.Context, segmentResources models.ClusterResources) (time.Duration, error) {
	var upperBound = 24 * time.Hour
	var baseline = 20 * time.Minute
	var magicConstant = time.Duration(2)

	var preset, err = gp.console.ResourcePresetByExtID(ctx, segmentResources.ResourcePresetExtID)
	if err != nil {
		return 0, err
	}

	if preset.IOLimit <= 0 {
		return 0, xerrors.Errorf("IO limit (%d) must be a positive integer", preset.IOLimit)
	}

	if segmentResources.DiskSize <= 0 {
		return 0, xerrors.Errorf("segmentResources.DiskSize (%d) must be a positive integer", segmentResources.DiskSize)
	}

	var timeout = baseline + time.Second*time.Duration(segmentResources.DiskSize/preset.IOLimit)*magicConstant
	if timeout > upperBound {
		timeout = upperBound
	}

	// 6 stands for 1st attempt and 5 retries
	return timeout * 6, nil
}

func (gp *Greenplum) RestoreHints(ctx context.Context, globalBackupID string) (gpmodels.RestoreHints, error) {
	cid, _, err := bmodels.DecodeGlobalBackupID(globalBackupID)
	if err != nil {
		return gpmodels.RestoreHints{}, err
	}

	var hints gpmodels.RestoreHints
	if err = gp.operator.ReadOnDeletedCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			// load source cluster info
			sourceCluster, masterResources, segmentResources, backup, majorVersion, err := gp.loadSourceClusterInfo(ctx, reader, globalBackupID)
			if err != nil {
				return err
			}
			var sourcePillar gppillars.Cluster
			err = sourceCluster.Pillar(&sourcePillar)
			if err != nil {
				return err
			}

			restoreTime := backup.CreatedAt.Add(time.Minute)
			hints = gpmodels.RestoreHints{
				RestoreHints: console.RestoreHints{
					Environment: sourceCluster.Environment,
					NetworkID:   sourceCluster.NetworkID,
					Version:     majorVersion,
					Time:        restoreTime,
				},
				MasterResources: console.RestoreResources{
					ResourcePresetID: masterResources.ResourcePresetExtID,
					DiskSize:         masterResources.DiskSize,
				},
				SegmentResources: console.RestoreResources{
					ResourcePresetID: segmentResources.ResourcePresetExtID,
					DiskSize:         segmentResources.DiskSize,
				},
				MasterHostCount:  sourcePillar.Data.Greenplum.MasterHostCount,
				SegmentHostCount: sourcePillar.Data.Greenplum.SegmentHostCount,
				SegmentInHost:    sourcePillar.Data.Greenplum.SegmentInHost,
			}
			return nil
		},
	); err != nil {
		return gpmodels.RestoreHints{}, err
	}

	return hints, nil
}

// loadSourceClusterInfo load source cluster info at the time of the provided backup
func (gp *Greenplum) loadSourceClusterInfo(
	ctx context.Context, reader clusterslogic.Reader, extBackupID string,
) (sourceCluster clusterslogic.Cluster, sourceResourceMaster, sourceResourceSegment models.ClusterResources, backup bmodels.Backup, majorVersion string, err error) {
	sourceCid, backupID, err := bmodels.DecodeGlobalBackupID(extBackupID)
	if err != nil {
		return clusterslogic.Cluster{}, models.ClusterResources{}, models.ClusterResources{}, bmodels.Backup{}, "", err
	}
	backup, err = gp.backups.BackupByClusterIDBackupID(ctx, sourceCid, backupID,
		func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
			return gp.listClusterBackups(ctx, reader, client, cid)
		})
	if err != nil {
		return clusterslogic.Cluster{}, models.ClusterResources{}, models.ClusterResources{}, bmodels.Backup{}, "", err
	}

	sourceCluster, sourceResourceMaster, err = reader.ClusterAndResourcesAtTime(ctx, sourceCid, backup.CreatedAt,
		clusters.TypeGreenplumCluster, hosts.RoleGreenplumMasterNode)
	if err != nil {
		return clusterslogic.Cluster{}, models.ClusterResources{}, models.ClusterResources{}, bmodels.Backup{}, "", err
	}

	major, err := getMajorVersionAtRev(ctx, reader, backup, sourceCid)
	if err != nil {
		return clusterslogic.Cluster{}, models.ClusterResources{}, models.ClusterResources{}, bmodels.Backup{}, "", err
	}

	_, sourceResourceSegment, err = reader.ClusterAndResourcesAtTime(ctx, sourceCid, backup.CreatedAt,
		clusters.TypeGreenplumCluster, hosts.RoleGreenplumSegmentNode)
	if err != nil {
		return clusterslogic.Cluster{}, models.ClusterResources{}, models.ClusterResources{}, bmodels.Backup{}, "", err
	}

	return sourceCluster, sourceResourceMaster, sourceResourceSegment, backup, major, nil
}

func (gp *Greenplum) FilterResourcePresetsAndGroupByRole(ctx context.Context, diskType string, flavorType string, folderID string) (map[string][]console.ResourcePreset, error) {
	resourcePresets, err := gp.console.GetResourcePresetsByClusterType(ctx, clusters.TypeGreenplumCluster, folderID, false)
	if err != nil {
		return nil, err
	}

	// Get zones
	zonesMap := make(map[string]bool)
	for _, preset := range resourcePresets {
		if _, ok := zonesMap[preset.Zone]; !ok {
			zonesMap[preset.Zone] = true
		}
	}
	zones := make([]string, 0)
	for k := range zonesMap {
		zones = append(zones, k)
	}

	// Get generations order by generation_id
	platforms, err := gp.console.GetPlatforms(ctx)
	if err != nil {
		return nil, err
	}
	sort.Slice(platforms[:], func(i, j int) bool {
		return platforms[i].Generation < platforms[j].Generation
	})

	filteredPresets := make([]console.ResourcePreset, 0)
	for _, preset := range resourcePresets {
		// stupid filter by one zone
		if preset.Zone != zones[0] {
			continue
		}
		// stupid filter by latest generation
		if preset.Generation != platforms[len(platforms)-1].Generation {
			continue
		}
		// filter by disktype
		if preset.DiskTypeExtID != diskType {
			continue
		}
		// remove not needed flavor types
		if preset.FlavorType != flavorType {
			continue
		}
		filteredPresets = append(filteredPresets, preset)
	}
	if len(filteredPresets) == 0 {
		return nil, xerrors.Errorf("no presets available after filter")
	}

	// group resource presets  by role
	response := make(map[string][]console.ResourcePreset)
	for _, preset := range filteredPresets {
		response[preset.Role.Stringified()] = append(response[preset.Role.Stringified()], preset)
	}
	//// Remove resource_presets with the same cpu count if such exists
	//// Need only for func tests
	for _, presets := range response {
		t := make(map[int64]console.ResourcePreset)
		for _, preset := range presets {
			t[preset.CPULimit] = preset
		}
		presets = presets[:0]
		for idx := range t {
			presets = append(presets, t[idx])
		}

		// order by cpu
		sort.Slice(presets[:], func(i, j int) bool {
			return presets[i].CPULimit < presets[j].CPULimit
		})
	}
	return response, nil
}

func (gp *Greenplum) GetRecommendedConfig(ctx context.Context, args *greenplum.RecommendedConfigArgs) (*greenplum.RecommendedConfig, error) {
	/*
		1. calculate resources needed to work with data size
		2. recommend disk type for master/segment
		4. recommend suitable resource preset
		5. recommend suitable segment host count
		6. recommend segments per host
		7. recommend disk size for master/segment
	*/

	const (
		MinMasterHosts                   = 2
		MasterHostDiskSizeMultiplication = 5

		MasterRoleName  = "greenplum_cluster.master_subcluster"
		SegmentRoleName = "greenplum_cluster.segment_subcluster"

		DedicatedHostLocalSSDMinSize  = 368 * 1024 * 1024 * 1024   // Gb
		DedicatedHostLocalSSDMaxSize  = 19200 * 1024 * 1024 * 1024 // 19200 Gb
		DedicatedHostLocalSSDStepSize = 368 * 1024 * 1024 * 1024

		SharedHostMaxCPU      = 24 // max cpu for segment on shared host
		SharedHostMaxDataSize = 20 * 1024 * 1024 * 1024 * 1024

		SharedHostNRDMinSize  = 93 * 1024 * 1024 * 1024
		SharedHostNRDMaxSize  = 8184 * 1024 * 1024 * 1024
		SharedHostNRDStepSize = 93 * 1024 * 1024 * 1024

		GEN2SharedHostLocalSSDMinSize  = 93 * 1024 * 1024 * 1024
		GEN2SharedHostLocalSSDMaxSize  = 1500 * 1024 * 1024 * 1024
		GEN2SharedHostLocalSSDStepSize = 93 * 1024 * 1024 * 1024

		GEN3SharedHostLocalSSDMinSize  = 368 * 1024 * 1024 * 1024
		GEN3SharedHostLocalSSDMaxSize  = 2944 * 1024 * 1024 * 1024
		GEN3SharedHostLocalSSDStepSize = 368 * 1024 * 1024 * 1024

		CompressionRatio      = 3
		CloudPerformanceRatio = 2
		// UnknownRatio from Excel need exploration from @vsgrab or remove it
		UnknownRatio = 1.1
		DiskRatio    = 100 * 1024 * 1024 * 1024 // 100 Gb
		MemRatio     = 10 * 1024 * 1024 * 1024  // 10 Gb
		CPURatio     = 1

		DiskFreeSpaceRatio = 1.3 // 30 % free space recommendation
		SegmentMirrorRatio = 2   // x2 space for mirroring

		GPSegmentMaxCPUCount  = 4 // allocated CPU for 1 GP segment
		GPSegmentMinHostCount = 4
	)
	ratios := &greenplum.RecommendedRatios{
		CompressionRatio:      CompressionRatio,
		CloudPerformanceRatio: CloudPerformanceRatio,
		UnknownRatio:          UnknownRatio,
		DiskRatio:             DiskRatio,
		MemRatio:              MemRatio,
		CPURatio:              CPURatio,
		DiskFreeSpaceRatio:    DiskFreeSpaceRatio,
		SegmentMirrorRatio:    SegmentMirrorRatio,
	}

	limits := &greenplum.HostLimits{
		SegmentMaxCPUCount:  GPSegmentMaxCPUCount,
		SegmentMinHostCount: GPSegmentMinHostCount,
		Dedicated: greenplum.Dedicated{
			LocalSSD: greenplum.LocalSSD{
				Min:  DedicatedHostLocalSSDMinSize,
				Max:  DedicatedHostLocalSSDMaxSize,
				Step: DedicatedHostLocalSSDStepSize,
			},
		},
		Shared: greenplum.Shared{
			ClusterMaxDataSize: SharedHostMaxDataSize,
			MaxCPU:             SharedHostMaxCPU,
			LocalSSD: map[int64]greenplum.LocalSSD{
				2: {
					Min:  GEN2SharedHostLocalSSDMinSize,
					Max:  GEN2SharedHostLocalSSDMaxSize,
					Step: GEN2SharedHostLocalSSDStepSize,
				},
				3: {
					Min:  GEN3SharedHostLocalSSDMinSize,
					Max:  GEN3SharedHostLocalSSDMaxSize,
					Step: GEN3SharedHostLocalSSDStepSize,
				},
			},
			NetworkSSDNonReplicated: greenplum.NetworkSSDNonReplicated{
				Min:  SharedHostNRDMinSize,
				Max:  SharedHostNRDMaxSize,
				Step: SharedHostNRDStepSize,
			},
		},
	}
	// calculate resources needed to work with datasize
	requestedResources := CalculateResourcesByUserRequest(args, ratios, limits)

	// try to recommend disk type
	diskTypeID := recommendDiskType(requestedResources.Dedicated, args.DiskTypeID)

	// get available flavors
	resourcePresetsByRole, err := gp.FilterResourcePresetsAndGroupByRole(ctx, diskTypeID, args.FlavorType, args.FolderID)
	if err != nil {
		return nil, err
	}

	// recommend segment hosts count
	segmentHosts, err := recommendSegmentHosts(requestedResources, resourcePresetsByRole, limits)
	if err != nil {
		return nil, err
	}

	// recommend flavor for segment
	roleFlavors, err := recommendFlavorExtID(resourcePresetsByRole, requestedResources, segmentHosts, limits)
	if err != nil {
		return nil, err
	}

	// recommend segments per host
	segmentsPerHost := recommendSegmentsInHost(roleFlavors[SegmentRoleName], limits)

	segmentFlavor := roleFlavors[SegmentRoleName]
	// recommend disk size for segments
	segmentHostDiskSize, err := recommendDiskSizeForSegmentHost(requestedResources, segmentFlavor, segmentHosts, limits)
	if err != nil {
		return nil, err
	}

	// recommend disk size for master
	masterDiskSize := roleFlavors[MasterRoleName].MemoryLimit * MasterHostDiskSizeMultiplication

	recommendedConfig := &greenplum.RecommendedConfig{
		UseDedicatedHosts: requestedResources.Dedicated,
		MasterHosts:       MinMasterHosts,
		SegmentsPerHost:   segmentsPerHost,
		SegmentHosts:      segmentHosts,
	}
	recommendedConfig.MasterConfig.Resource.Generation = roleFlavors[MasterRoleName].Generation
	recommendedConfig.MasterConfig.Resource.Type = roleFlavors[MasterRoleName].FlavorType
	recommendedConfig.MasterConfig.Resource.ResourcePresetExtID = roleFlavors[MasterRoleName].ExtID
	recommendedConfig.MasterConfig.Resource.DiskTypeExtID = diskTypeID
	recommendedConfig.MasterConfig.Resource.DiskSize = masterDiskSize

	recommendedConfig.SegmentConfig.Resource.Generation = roleFlavors[SegmentRoleName].Generation
	recommendedConfig.SegmentConfig.Resource.Type = roleFlavors[SegmentRoleName].FlavorType
	recommendedConfig.SegmentConfig.Resource.ResourcePresetExtID = roleFlavors[SegmentRoleName].ExtID
	recommendedConfig.SegmentConfig.Resource.DiskTypeExtID = diskTypeID
	recommendedConfig.SegmentConfig.Resource.DiskSize = segmentHostDiskSize

	return recommendedConfig, nil
}

func getTaskCreationInfo() common.TaskCreationInfo {
	return common.TaskCreationInfo{
		ClusterModifyTask:      gpmodels.TaskTypeClusterModify,
		ClusterModifyOperation: gpmodels.OperationTypeClusterModify,

		MetadataUpdateTask:      gpmodels.TaskTypeMetadataUpdate,
		MetadataUpdateOperation: gpmodels.OperationTypeMetadataUpdate,

		SearchService: greenplumService,
	}
}

func getMajorVersionAtRev(ctx context.Context, reader clusterslogic.Reader, backup bmodels.Backup, sourceCid string) (string, error) {
	versions, err := reader.ClusterVersionsAtTime(ctx, sourceCid, backup.CreatedAt)
	if err != nil {
		return "", xerrors.Errorf("get cluster versions: %w", err)
	}

	return getMajorVersionByVersions(versions)
}

func getMajorVersion(ctx context.Context, reader clusterslogic.Reader, cid string) (string, error) {
	versions, err := reader.ClusterVersions(ctx, cid)
	if err != nil {
		return "", xerrors.Errorf("get cluster versions: %w", err)
	}

	return getMajorVersionByVersions(versions)
}

func getMajorVersionByVersions(versions []console.Version) (string, error) {
	for _, version := range versions {
		if version.Component == "greenplum" {
			return version.MajorVersion, nil
		}
	}

	return "", xerrors.Errorf("No entry for greenplum component version in cluster versions")
}
func recommendDiskSizeForSegmentHost(req *greenplum.CalculatedResources, segmentFlavor console.ResourcePreset, segmentHosts int64, limits *greenplum.HostLimits) (int64, error) {
	/*
		1. On dedicated hosts we should use only local-ssd and return max-size
		2. On shared hosts calculate needed resources
	*/

	if req.Dedicated {
		return limits.Dedicated.LocalSSD.Max, nil
	}
	size := req.DISK / segmentHosts
	generation := segmentFlavor.Generation
	switch segmentFlavor.DiskTypeExtID {
	case resources.LocalSSD:
		if size > limits.Shared.LocalSSD[generation].Max {
			size = limits.Shared.LocalSSD[generation].Max
		} else if size < limits.Shared.LocalSSD[generation].Min {
			size = limits.Shared.LocalSSD[generation].Min
		} else {
			size = size - size%limits.Shared.LocalSSD[generation].Step + limits.Shared.LocalSSD[generation].Step
		}
	case resources.NetworkSSDNonreplicated:
		if size > limits.Shared.NetworkSSDNonReplicated.Max {
			size = limits.Shared.NetworkSSDNonReplicated.Max
		} else if size < limits.Shared.NetworkSSDNonReplicated.Min {
			size = limits.Shared.NetworkSSDNonReplicated.Min
		} else {
			size = size - size%limits.Shared.NetworkSSDNonReplicated.Step + limits.Shared.NetworkSSDNonReplicated.Step
		}
	default:
		return 0, xerrors.Errorf("unknown disk type %v", segmentFlavor.DiskTypeExtID)
	}
	return size, nil
}

func CalculateResourcesByUserRequest(args *greenplum.RecommendedConfigArgs, ratios *greenplum.RecommendedRatios, limits *greenplum.HostLimits) *greenplum.CalculatedResources {
	/*
		1. use formula 1 / 10 / 100 to calculate resource pool needed for customer.
		it means that for raw 1000 Gb data we should use 10 cpu, 100 GB RAM.
		2. Apply ratios from excel

	*/
	cpu := RoundUpToEven(float64(args.DataSize/ratios.CompressionRatio*ratios.CloudPerformanceRatio/ratios.DiskRatio) * ratios.UnknownRatio)
	mem := RoundUpToEven(float64(args.DataSize/ratios.CompressionRatio*ratios.CloudPerformanceRatio/ratios.MemRatio) * ratios.UnknownRatio)
	disk := RoundUpToEven(float64(args.DataSize/ratios.CompressionRatio*ratios.CPURatio) * ratios.DiskFreeSpaceRatio * float64(ratios.SegmentMirrorRatio))
	dedicated := false
	if disk > limits.Shared.ClusterMaxDataSize {
		dedicated = true
	}
	if args.UseDedicatedHosts {
		dedicated = true
	}
	calculatedResources := &greenplum.CalculatedResources{
		CPU:       cpu,
		MEM:       mem,
		DISK:      disk,
		Dedicated: dedicated,
	}
	return calculatedResources
}

func recommendSegmentHosts(calculatedResources *greenplum.CalculatedResources, resourcePresets map[string][]console.ResourcePreset, limits *greenplum.HostLimits) (int64, error) {
	/*
		1. For dedicated hosts use only local-ssd disk type.
		2. For shared hosts use nrd or local-ssd disk types and calculate size.
		3. For shared hosts max flavor <= limits.Shared.MaxCPU, should recommend to scale up by increasing segment hosts
	*/

	const (
		SegmentRoleName = "greenplum_cluster.segment_subcluster"
	)
	var segmentHostCount int64
	if len(resourcePresets[SegmentRoleName]) == 0 {
		return 0, xerrors.Errorf("no flavors found for %v", SegmentRoleName)
	}
	DedicatedHostCPUMaxFlavor := resourcePresets[SegmentRoleName][len(resourcePresets[SegmentRoleName])-1].CPULimit

	if calculatedResources.Dedicated {
		if calculatedResources.CPU > limits.SegmentMinHostCount*DedicatedHostCPUMaxFlavor {
			segmentHostCount = calculatedResources.CPU / DedicatedHostCPUMaxFlavor
		} else {
			segmentHostCount = limits.SegmentMinHostCount
		}
	} else {
		if calculatedResources.CPU/limits.Shared.MaxCPU < limits.SegmentMinHostCount {
			segmentHostCount = limits.SegmentMinHostCount
		} else {
			segmentHostCount = RoundUpToEven(float64(calculatedResources.CPU) / float64(limits.Shared.MaxCPU))
		}
	}
	return segmentHostCount, nil
}
func RoundUpToEven(i float64) int64 {
	r := int64(math.Ceil(i))
	if r == 0 {
		r = 1
	}
	if r%2 != 0 {
		r += 1
	}
	return r
}

func recommendDiskType(dedicatedHost bool, userWantedDiskType string) string {
	/*
		On dedicated hosts use only local-ssd disk types.
	*/
	if dedicatedHost {
		return resources.LocalSSD
	}
	return userWantedDiskType
}

func recommendSegmentsInHost(resourcePreset console.ResourcePreset, limits *greenplum.HostLimits) int64 {
	segmentsPerHost := resourcePreset.CPULimit / limits.SegmentMaxCPUCount
	if segmentsPerHost <= 0 {
		segmentsPerHost = 1
	}
	return segmentsPerHost
}

func recommendFlavorExtID(resMap map[string][]console.ResourcePreset, calculatedResources *greenplum.CalculatedResources, segmentHostCount int64, limits *greenplum.HostLimits) (map[string]console.ResourcePreset, error) {
	/*
		1. For dedicated servers return max flavor
		2. For shared hosts get the nearest flavor by cpu.
	*/

	roles := make([]string, 0)
	for k := range resMap {
		roles = append(roles, k)
	}

	response := make(map[string]console.ResourcePreset)
	if calculatedResources.Dedicated {
		for _, role := range roles {
			last := len(resMap[role]) - 1
			response[role] = resMap[role][last]
		}
	} else {
		neededCPUPerSegment := RoundUpToEven(float64(calculatedResources.CPU) / float64(segmentHostCount))
		for _, role := range roles {
			nearest := -1
			var value int64
			value = math.MaxInt64
			for idx, preset := range resMap[role] {
				if int64(math.Abs(float64(preset.CPULimit-neededCPUPerSegment))) <= value && preset.CPULimit <= limits.Shared.MaxCPU {
					value = int64(math.Abs(float64(preset.CPULimit - neededCPUPerSegment)))
					nearest = idx
				}
			}
			if nearest == -1 {
				return nil, xerrors.Errorf("Could not recommend flavor")
			}
			response[role] = resMap[role][nearest]
		}
	}
	return response, nil
}
