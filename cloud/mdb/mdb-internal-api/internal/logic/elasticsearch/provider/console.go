package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/esmodels"
	cls "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	clusters "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

func (es *ElasticSearch) AllowedEditions(ctx context.Context) ([]esmodels.Edition, error) {
	return es.allowedEditions, nil
}

func (es *ElasticSearch) SupportedVersions(ctx context.Context) esmodels.SupportedVersions {
	return es.supportedVersions
}

func (es *ElasticSearch) EstimateCreateCluster(ctx context.Context, args elasticsearch.CreateClusterArgs) (console.BillingEstimate, error) {
	var res console.BillingEstimate
	err := es.operator.ReadOnFolder(ctx, args.FolderExtID,
		func(ctx context.Context, session sessions.Session, reader cls.Reader) error {
			hostsCount := len(args.HostSpec)
			hostBillingSpecs := make([]cls.HostBillingSpec, 0, hostsCount)
			for i := 0; i < hostsCount; i++ {
				role := args.HostSpec[i].Role
				var hostResources models.ClusterResources
				switch {
				case role == hosts.RoleElasticSearchDataNode:
					if !args.ConfigSpec.Config.Valid || !args.ConfigSpec.Config.Value.DataNode.Resources.IsSet() {
						return semerr.InvalidInput("data node resources not defined")
					}
					if err := args.ConfigSpec.Config.Value.DataNode.Resources.Validate(true); err != nil {
						return err
					}
					hostResources = args.ConfigSpec.Config.Value.DataNode.Resources.MustOptionals()
				case role == hosts.RoleElasticSearchMasterNode:
					if !args.ConfigSpec.Config.Valid || !args.ConfigSpec.Config.Value.MasterNode.Valid ||
						!args.ConfigSpec.Config.Value.MasterNode.Value.Resources.IsSet() {
						return semerr.InvalidInput("master node resources not defined")
					}
					if err := args.ConfigSpec.Config.Value.MasterNode.Value.Resources.Validate(true); err != nil {
						return err
					}
					hostResources = args.ConfigSpec.Config.Value.MasterNode.Value.Resources.MustOptionals()
				}
				hostBillingSpecs = append(hostBillingSpecs, cls.HostBillingSpec{
					HostRole:         role,
					ClusterResources: hostResources,
					AssignPublicIP:   args.HostSpec[i].AssignPublicIP,
				})
			}

			var err error
			res, err = reader.EstimateBilling(ctx, args.FolderExtID, clusters.TypeElasticSearch, hostBillingSpecs, environment.CloudTypeYandex)
			return err
		},
		cls.WithPermission(esmodels.PermClustersGet),
	)
	if err != nil {
		return res, err
	}

	edition, err := esmodels.ParseEdition(args.ConfigSpec.Edition.Value.String())
	if err != nil {
		return console.BillingEstimate{}, err
	}
	for i := range res.Metrics {
		res.Metrics[i].Tags.Edition = edition.String()
	}

	return res, err
}
