package provider

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/metastore"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/metastore/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/metastore/provider/internal/pillars"
	common_models "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

type createClusterImplArgs struct {
	Name               string
	CloudType          environment.CloudType
	FolderExtID        string
	ConfigSpec         models.ClusterConfigSpec
	SubnetIDs          []string
	NetworkID          string
	Description        string
	Labels             clusters.Labels
	HostGroupIDs       []string
	DeletionProtection bool
	SecurityGroupIDs   []string
	Version            string
	MinServersPerZone  int64
	MaxServersPerZone  int64
}

func (args createClusterImplArgs) Validate() error {
	if args.FolderExtID == "" {
		return semerr.InvalidInput("folder id must be specified") // TODO use resource model when go to DataCloud
	}
	if args.Name == "" {
		return semerr.InvalidInput("cluster name must be specified")
	}
	if args.MinServersPerZone < 1 {
		return semerr.InvalidInput("minimum servers per zone number must be at least 1")
	}
	if args.MaxServersPerZone > 16 {
		return semerr.InvalidInput("maximum servers per zone number must be at most 16")
	}
	if len(args.SubnetIDs) == 0 {
		return semerr.InvalidInput("at least one subnet should be specified")
	}

	if err := args.ConfigSpec.Validate(); err != nil {
		return err
	}

	//return models.ClusterNameValidator.ValidateString(args.Name)
	return nil
}

func (ms *Metastore) CreateMDBCluster(ctx context.Context, args metastore.CreateMDBClusterArgs) (operations.Operation, error) {
	return ms.operator.Create(ctx, args.FolderExtID,
		func(ctx context.Context, session sessions.Session, creator clusterslogic.Creator) (clusters.Cluster, operations.Operation, error) {
			if !session.FeatureFlags.Has(models.MetastoreClusterFeatureFlag) {
				return clusters.Cluster{}, operations.Operation{}, semerr.Authorization("operation is not allowed for this cloud")
			}
			createArgs, err := ms.convertMdbArgs(ctx, session, args)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			return ms.createClusterImpl(ctx, session, creator, createArgs)
		},
	)
}

func (ms *Metastore) createClusterImpl(ctx context.Context, session sessions.Session, creator clusterslogic.Creator,
	args createClusterImplArgs) (clusters.Cluster, operations.Operation, error) {
	if err := args.Validate(); err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	cl, privKey, err := creator.CreateCluster(ctx, common_models.CreateClusterArgs{
		Name:               args.Name,
		ClusterType:        clusters.TypeMetastore,
		Environment:        environment.SaltEnvProd,
		FolderID:           session.FolderCoords.FolderID,
		Description:        args.Description,
		Labels:             args.Labels,
		HostGroupIDs:       args.HostGroupIDs,
		DeletionProtection: args.DeletionProtection,
	})
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create cluster: %w", err)
	}

	subcluster, err := creator.CreateKubernetesSubCluster(ctx, common_models.CreateSubClusterArgs{
		ClusterID: cl.ClusterID,
		Name:      models.MetastoreSubClusterName,
		Roles:     []hosts.Role{hosts.RoleMetastore},
		Revision:  cl.Revision,
	})
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create metastore subcluster: %w", err)
	}

	// Create cluster pillar
	pillar, err := ms.makeClusterPillar(ctx, args, privKey, cl)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
	}

	if len(args.SecurityGroupIDs) > 0 {
		args.SecurityGroupIDs = slices.DedupStrings(args.SecurityGroupIDs)
		// validate security groups after NetworkID validation, cause we validate SG above network
		if err := ms.compute.ValidateSecurityGroups(ctx, args.SecurityGroupIDs, args.NetworkID); err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}
	}

	err = creator.AddClusterPillar(ctx, cl.ClusterID, cl.Revision, pillar)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
	}

	opts := tasks.CreateTaskArgs(map[string]interface{}{
		"subcid": subcluster.SubClusterID,
	})

	op, err := ms.tasks.CreateCluster(
		ctx,
		session,
		cl.ClusterID,
		cl.Revision,
		models.TaskTypeClusterCreate,
		models.OperationTypeClusterCreate,
		models.MetadataCreateCluster{},
		optional.String{},
		args.SecurityGroupIDs,
		metastoreService,
		searchAttributesExtractor,
		opts,
	)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	return cl, op, nil
}

func searchAttributesExtractor(cluster clusterslogic.Cluster) (map[string]interface{}, error) {
	searchAttributes := make(map[string]interface{})
	return searchAttributes, nil
}

func (ms *Metastore) convertMdbArgs(ctx context.Context, session sessions.Session, args metastore.CreateMDBClusterArgs) (createClusterImplArgs, error) {
	result := createClusterImplArgs{
		Name:               args.Name,
		FolderExtID:        args.FolderExtID,
		CloudType:          environment.CloudTypeYandex,
		SubnetIDs:          args.SubnetIDs,
		Description:        args.Description,
		Labels:             args.Labels,
		HostGroupIDs:       args.HostGroupIDs,
		DeletionProtection: args.DeletionProtection,
		SecurityGroupIDs:   args.SecurityGroupIDs,
		MinServersPerZone:  args.MinServersPerZone,
		MaxServersPerZone:  args.MaxServersPerZone,
		Version:            args.Version,
	}
	return result, nil
}

func (ms *Metastore) makeClusterPillar(ctx context.Context, args createClusterImplArgs, privateKey []byte, cluster clusters.Cluster) (*pillars.Cluster, error) {
	pillar := pillars.NewCluster()

	pillar.Data.ZoneIDs = make([]string, 0)
	for _, subnetID := range args.SubnetIDs {
		subnet, err := ms.compute.Subnet(ctx, subnetID)
		if err != nil {
			return pillar, err
		}
		if args.NetworkID != "" && args.NetworkID != subnet.NetworkID {
			return pillar, xerrors.Errorf(
				"all subnets must belong to the same network but specified subnets belong to networks %s and %s",
				args.NetworkID,
				subnet.NetworkID,
			)
		}
		pillar.Data.ZoneIDs = append(pillar.Data.ZoneIDs, subnet.ZoneID)
		args.NetworkID = subnet.NetworkID
	}

	pillar.Data.Version = args.Version
	pillar.Data.UserSubnetIDs = args.SubnetIDs
	pillar.Data.MinServersPerZone = args.MinServersPerZone
	pillar.Data.MaxServersPerZone = args.MaxServersPerZone
	pillar.Data.NetworkID = args.NetworkID

	pillar.Data.ServiceSubnetIDs = ms.cfg.Metastore.ServiceSubnetIDs
	pillar.Data.ServiceAccountID = ms.cfg.Metastore.KubernetesClusterServiceAccountID
	pillar.Data.NodeServiceAccountID = ms.cfg.Metastore.KubernetesNodeServiceAccountID
	pillar.Data.KubernetesClusterID = ms.cfg.Metastore.KubernetesClusterID
	pillar.Data.PostgresqlClusterID = ms.cfg.Metastore.PostgresqlClusterID
	pillar.Data.PostgresqlHostname = ms.cfg.Metastore.PostgresqlHostname

	pillar.Data.KubernetesNamespaceName = fmt.Sprintf("metastore-%s", cluster.ClusterID)
	pillar.Data.DBName = fmt.Sprintf("db_%s", cluster.ClusterID)
	pillar.Data.DBUserName = fmt.Sprintf("user_%s", cluster.ClusterID)
	password, err := crypto.GenerateEncryptedPassword(ms.cryptoProvider, models.PasswordLen, nil)
	if err != nil {
		return nil, err
	}
	pillar.Data.DBUserPassword = password

	return pillar, nil
}
