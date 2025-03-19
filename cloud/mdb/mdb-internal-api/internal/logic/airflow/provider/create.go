package provider

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/airflow"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/airflow/afmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/airflow/provider/internal/afpillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

const (
	passwordGenLen = 16
)

func (af *Airflow) CreateMDBCluster(ctx context.Context, args airflow.CreateMDBClusterArgs) (operations.Operation, error) {
	return af.operator.Create(ctx, args.FolderExtID,
		func(ctx context.Context, session sessions.Session, creator clusterslogic.Creator) (clusters.Cluster, operations.Operation, error) {
			if !session.FeatureFlags.Has(afmodels.AirflowClusterFeatureFlag) {
				return clusters.Cluster{}, operations.Operation{}, semerr.Authorization("operation is not allowed for this cloud")
			}

			return af.createClusterImpl(ctx, session, creator, args)
		},
	)
}

func (af *Airflow) findNetwork(ctx context.Context, subnetIDs []string) (string, error) {

	networkID := optional.String{}
	for _, subnetID := range subnetIDs {
		subnet, err := af.compute.Subnet(ctx, subnetID)
		if err != nil {
			return "", err
		}
		if !networkID.Valid {
			networkID.Set(subnet.NetworkID)
		} else {
			if networkID.String != subnet.NetworkID {
				return "", xerrors.Errorf(
					"all subnets must belong to the same network but specified subnets belong to networks %s and %s",
					networkID.String,
					subnet.NetworkID,
				)
			}
		}
	}
	if !networkID.Valid {
		return "", xerrors.Errorf("Network ID is not found")
	}
	return networkID.String, nil
}

func (af *Airflow) createClusterImpl(ctx context.Context, session sessions.Session, creator clusterslogic.Creator,
	args airflow.CreateMDBClusterArgs) (clusters.Cluster, operations.Operation, error) {
	if err := args.Validate(); err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	networkID, err := af.findNetwork(ctx, args.Network.SubnetIDs)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	cl, _, err := creator.CreateCluster(ctx, models.CreateClusterArgs{
		Name:               args.Name,
		ClusterType:        clusters.TypeAirflow,
		Environment:        args.Environment,
		NetworkID:          networkID,
		FolderID:           session.FolderCoords.FolderID,
		Description:        args.Description,
		Labels:             args.Labels,
		DeletionProtection: args.DeletionProtection,
	})
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create cluster: %w", err)
	}

	subcluster, err := creator.CreateKubernetesSubCluster(ctx, models.CreateSubClusterArgs{
		ClusterID: cl.ClusterID,
		Name:      "Airflow Node group",
		Roles:     []hosts.Role{hosts.RoleAirflow},
		Revision:  cl.Revision,
	})
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create airflow subcluster: %w", err)
	}

	// Create cluster pillar
	pillar, err := af.makeClusterPillar(args)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("failed to create pillar: %w", err)
	}

	if len(args.Network.SecurityGroupIDs) > 0 {
		args.Network.SecurityGroupIDs = slices.DedupStrings(args.Network.SecurityGroupIDs)
		// validate security groups after NetworkID validation, cause we validate SG above network
		if err := af.compute.ValidateSecurityGroups(ctx, args.Network.SecurityGroupIDs, networkID); err != nil {
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

	op, err := af.tasks.CreateCluster(
		ctx,
		session,
		cl.ClusterID,
		cl.Revision,
		afmodels.TaskTypeClusterCreate,
		afmodels.OperationTypeClusterCreate,
		afmodels.MetadataCreateCluster{},
		optional.String{},
		args.Network.SecurityGroupIDs,
		airflowService,
		searchAttributesExtractor,
		opts,
	)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	return cl, op, nil
}

func (af *Airflow) makeClusterPillar(args airflow.CreateMDBClusterArgs) (*afpillars.Cluster, error) {
	pillar := afpillars.NewCluster()

	pillar.Data.S3Bucket = args.CodeSync.Bucket

	// TODO: Un-hardcode it
	pillar.Data.Kubernetes.ClusterID = af.cfg.Airflow.KubernetesClusterID
	pillar.Data.Kubernetes.SubnetIDs = args.Network.SubnetIDs

	pillar.Values.Version = args.ConfigSpec.Version
	pillar.Values.Config = args.ConfigSpec.Airflow.Config
	pillar.Values.Webserver.Replicas = args.ConfigSpec.Webserver.Count
	pillar.Values.Webserver.Resources = afpillars.FromModel(args.ConfigSpec.Webserver.Resources)
	pillar.Values.Scheduler.Replicas = args.ConfigSpec.Scheduler.Count
	pillar.Values.Scheduler.Resources = afpillars.FromModel(args.ConfigSpec.Scheduler.Resources)
	if args.ConfigSpec.Triggerer != nil {
		pillar.Values.Triggerer.Replicas = args.ConfigSpec.Triggerer.Count
		pillar.Values.Triggerer.Resources = afpillars.FromModel(args.ConfigSpec.Triggerer.Resources)
	}
	pillar.Values.Worker.KEDA = afpillars.KEDAConfig{
		MinReplicaCount:    args.ConfigSpec.Worker.MinCount,
		MaxReplicaCount:    args.ConfigSpec.Worker.MaxCount,
		CooldownPeriodSec:  int64(args.ConfigSpec.Worker.CooldownPeriod.Truncate(time.Second) / time.Second),
		PollingIntervalSec: int64(args.ConfigSpec.Worker.PollingInterval.Truncate(time.Second) / time.Second),
	}
	pillar.Values.Worker.Resources = afpillars.FromModel(args.ConfigSpec.Worker.Resources)

	return pillar, nil
}
