package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	cls "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch/osmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	clusters "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

func (es *OpenSearch) SupportedVersions(ctx context.Context) osmodels.SupportedVersions {
	return es.supportedVersions
}

func (es *OpenSearch) EstimateCreateCluster(ctx context.Context, args opensearch.CreateClusterArgs) (console.BillingEstimate, error) {
	var res console.BillingEstimate
	err := es.operator.ReadOnFolder(ctx, args.FolderExtID,
		func(ctx context.Context, session sessions.Session, reader cls.Reader) error {
			hostsCount := len(args.HostSpec)
			hostBillingSpecs := make([]cls.HostBillingSpec, 0, hostsCount)
			for i := 0; i < hostsCount; i++ {
				role := args.HostSpec[i].Role
				var hostResources models.ClusterResources
				switch {
				case role == hosts.RoleOpenSearchDataNode:
					if !args.ConfigSpec.Config.Valid || !args.ConfigSpec.Config.Value.DataNode.Resources.IsSet() {
						return semerr.InvalidInput("data node resources not defined")
					}
					if err := args.ConfigSpec.Config.Value.DataNode.Resources.Validate(true); err != nil {
						return err
					}
					hostResources = args.ConfigSpec.Config.Value.DataNode.Resources.MustOptionals()
				case role == hosts.RoleOpenSearchMasterNode:
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
			res, err = reader.EstimateBilling(ctx, args.FolderExtID, clusters.TypeOpenSearch, hostBillingSpecs, environment.CloudTypeYandex)
			return err
		},
		cls.WithPermission(osmodels.PermClustersGet),
	)
	if err != nil {
		return res, err
	}

	return res, err
}
