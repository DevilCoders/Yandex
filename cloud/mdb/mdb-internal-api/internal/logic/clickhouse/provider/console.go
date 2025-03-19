package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/provider/internal/chpillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (ch *ClickHouse) Version(ctx context.Context, metadb metadb.Backend, cid string) (string, error) {
	scs, err := metadb.SubClustersByClusterID(ctx, cid)
	if err != nil {
		return "", err
	}
	for _, sc := range scs {
		if sc.Name == chmodels.CHSubClusterName {
			var pillar chpillars.SubClusterCH
			if err := pillar.UnmarshalPillar(sc.Pillar); err != nil {
				return "", err
			}

			return chmodels.CutVersionToMajor(pillar.Data.ClickHouse.Version), nil
		}
	}
	return "", xerrors.Errorf("failed to get cluster version %q", cid)
}

func (ch ClickHouse) VersionIDs() []string {
	var versionNames []string

	for _, v := range ch.versions.Versions {
		versionNames = append(versionNames, v.ID)
	}

	return versionNames
}

func (ch ClickHouse) Versions() []logic.Version {
	return ch.versions.Versions
}

func (ch ClickHouse) DefaultVersion() logic.Version {
	return ch.versions.Default
}

func (ch *ClickHouse) EstimateCreateMDBCluster(ctx context.Context, args clickhouse.CreateMDBClusterArgs) (billingEstimate console.BillingEstimate, err error) {
	err = ch.operator.FakeCreate(ctx, args.FolderExtID,
		func(ctx context.Context, session sessions.Session, creator clusterslogic.Creator) error {
			var clusterArgs createClusterImplArgs
			clusterArgs, err = ch.prepareMDBClusterCreate(ctx, session, creator, args)
			if err != nil {
				return err
			}

			var (
				chResources = clusterArgs.ClusterSpec.ClickHouseResources.MustOptionals()
				zkResources models.ClusterResources
			)
			if clusterArgs.ClusterHosts.NeedZookeeper() {
				zkResources = clusterArgs.ClusterSpec.ZookeeperResources.MustOptionals()
			}

			hostBillingSpecs := make([]clusterslogic.HostBillingSpec, clusterArgs.ClusterHosts.Count())

			clusterHosts := append(clusterArgs.ClusterHosts.ClickHouseNodes, clusterArgs.ClusterHosts.ZooKeeperNodes...)
			for i, host := range clusterHosts {
				hostResources := models.ClusterResources{}
				switch host.HostRole {
				case hosts.RoleClickHouse:
					hostResources = chResources
				case hosts.RoleZooKeeper:
					hostResources = zkResources
				}

				hostBillingSpecs[i] = clusterslogic.HostBillingSpec{
					HostRole:         host.HostRole,
					ClusterResources: hostResources,
					AssignPublicIP:   host.AssignPublicIP,
				}
			}

			billingEstimate, err = creator.EstimateBilling(ctx, args.FolderExtID, clusters.TypeClickHouse, hostBillingSpecs, environment.CloudTypeYandex)
			return err
		})

	return billingEstimate, err
}

func (ch *ClickHouse) EstimateCreateDCCluster(ctx context.Context, args clickhouse.CreateDataCloudClusterArgs) (billingEstimate console.BillingEstimate, err error) {
	err = ch.operator.FakeCreate(ctx, args.ProjectID,
		func(ctx context.Context, session sessions.Session, creator clusterslogic.Creator) error {
			var clusterArgs createClusterImplArgs
			clusterArgs, err = ch.prepareDataCloudClusterCreateEstimation(ctx, session, creator, args)
			if err != nil {
				return err
			}

			hostBillingSpecs := make([]clusterslogic.HostBillingSpec, len(clusterArgs.ClusterHosts.ClickHouseNodes))

			for i, host := range clusterArgs.ClusterHosts.ClickHouseNodes {
				hostBillingSpecs[i] = clusterslogic.HostBillingSpec{
					HostRole:         host.HostRole,
					ClusterResources: clusterArgs.ClusterSpec.ClickHouseResources.MustOptionals(),
					AssignPublicIP:   host.AssignPublicIP,
				}
			}

			billingEstimate, err = creator.EstimateBilling(ctx, args.ProjectID, clusters.TypeClickHouse, hostBillingSpecs, environment.CloudTypeAWS)

			for i := 0; i < len(billingEstimate.Metrics); i++ {
				billingEstimate.Metrics[i].Tags.CloudRegion = args.RegionID
				billingEstimate.Metrics[i].Tags.CloudProvider = string(args.CloudType)
			}

			return err
		})

	return billingEstimate, err
}
