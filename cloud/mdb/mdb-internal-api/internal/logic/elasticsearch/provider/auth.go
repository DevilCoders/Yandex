package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/esmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/provider/internal/espillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
)

func (es *ElasticSearch) AuthProvider(ctx context.Context, cid string, name string) (*esmodels.AuthProvider, error) {
	var res *esmodels.AuthProvider
	err := es.operator.ReadOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			pillar := espillars.NewCluster()
			if err = cl.Pillar(pillar); err != nil {
				return err
			}

			providers, err := pillar.AuthProviders()
			if err != nil {
				return err
			}

			res, err = providers.Find(name)

			return err
		},
		clusterslogic.WithPermission(esmodels.PermClustersGet),
	)
	return res, err
}

func (es *ElasticSearch) AuthProviders(ctx context.Context, cid string) (*esmodels.AuthProviders, error) {
	var res *esmodels.AuthProviders
	err := es.operator.ReadOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			pillar := espillars.NewCluster()
			if err = cl.Pillar(pillar); err != nil {
				return err
			}

			res, err = pillar.AuthProviders()

			return err
		},
		clusterslogic.WithPermission(esmodels.PermClustersGet),
	)
	return res, err
}

func names(providers ...*esmodels.AuthProvider) []string {
	res := make([]string, 0, esmodels.MaxAuthProviders)
	for _, p := range providers {
		res = append(res, p.Name)
	}
	return res
}

func (es *ElasticSearch) AddAuthProviders(ctx context.Context, cid string, providers ...*esmodels.AuthProvider) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := espillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			ap, err := pillar.AuthProviders()
			if err != nil {
				return operations.Operation{}, err
			}

			for _, p := range providers {
				if !p.Type.AllowedForEdition(pillar.Edition()) {
					return operations.Operation{}, semerr.FailedPreconditionf("auth provider type %q not allowed for you license", p.Type)
				}
			}

			if err := ap.Add(providers...); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.SetAuthProviders(ap); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			return es.applyChanges(ctx, session, cluster,
				esmodels.OperationTypeAddAuthProviders, esmodels.MetadataAddAuthProviders{
					Names: names(providers...),
				})
		},
		clusterslogic.WithPermission(esmodels.PermClustersUpdate),
	)
}

// run task for apply cluster changes
func (es *ElasticSearch) applyChanges(
	ctx context.Context,
	session sessions.Session,
	cluster clusterslogic.Cluster,
	optype operations.Type,
	metadata operations.Metadata,
) (operations.Operation, error) {
	return es.tasks.CreateTask(
		ctx,
		session,
		tasks.CreateTaskArgs{
			ClusterID:     cluster.ClusterID,
			FolderID:      session.FolderCoords.FolderID,
			Auth:          session.Subject,
			TaskType:      esmodels.TaskTypeClusterModify,
			OperationType: optype,
			Revision:      cluster.Revision,
			Metadata:      metadata,
			Timeout:       optional.NewDuration(updateClusterDefaultTimeout),
			TaskArgs: map[string]interface{}{
				"restart": true,
			},
		})
}

func (es *ElasticSearch) UpdateAuthProviders(ctx context.Context, cid string, providers ...*esmodels.AuthProvider) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := espillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			for _, p := range providers {
				if !p.Type.AllowedForEdition(pillar.Edition()) {
					return operations.Operation{}, semerr.FailedPreconditionf("auth provider type %q not allowed for you license", p.Type)
				}
			}

			ap := esmodels.NewAuthProviders()
			if err := ap.Add(providers...); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.SetAuthProviders(ap); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			return es.applyChanges(ctx, session, cluster,
				esmodels.OperationTypeUpdateAuthProviders, esmodels.MetadataUpdateAuthProviders{
					Names: names(providers...),
				})
		},
		clusterslogic.WithPermission(esmodels.PermClustersUpdate),
	)
}

func (es *ElasticSearch) DeleteAuthProviders(ctx context.Context, cid string, names ...string) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := espillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			ap, err := pillar.AuthProviders()
			if err != nil {
				return operations.Operation{}, err
			}

			if err := ap.Delete(names...); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.SetAuthProviders(ap); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			return es.applyChanges(ctx, session, cluster,
				esmodels.OperationTypeDeleteAuthProviders, esmodels.MetadataDeleteAuthProviders{
					Names: names,
				})
		},
		clusterslogic.WithPermission(esmodels.PermClustersUpdate),
	)
}

func (es *ElasticSearch) UpdateAuthProvider(ctx context.Context, cid string, name string, provider *esmodels.AuthProvider) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := espillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			if !provider.Type.AllowedForEdition(pillar.Edition()) {
				return operations.Operation{}, semerr.FailedPreconditionf("auth provider type %q not allowed for you license", provider.Type)
			}

			ap, err := pillar.AuthProviders()
			if err != nil {
				return operations.Operation{}, err
			}

			if err := ap.Update(name, provider); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.SetAuthProviders(ap); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			return es.applyChanges(ctx, session, cluster,
				esmodels.OperationTypeUpdateAuthProviders, esmodels.MetadataUpdateAuthProviders{
					Names: names(provider),
				})
		},
		clusterslogic.WithPermission(esmodels.PermClustersUpdate),
	)
}
