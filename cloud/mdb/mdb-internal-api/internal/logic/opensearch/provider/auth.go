package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch/osmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch/provider/internal/ospillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
)

func (es *OpenSearch) AuthProvider(ctx context.Context, cid string, name string) (*osmodels.AuthProvider, error) {
	var res *osmodels.AuthProvider
	err := es.operator.ReadOnCluster(ctx, cid, clusters.TypeOpenSearch,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			pillar := ospillars.NewCluster()
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
		clusterslogic.WithPermission(osmodels.PermClustersGet),
	)
	return res, err
}

func (es *OpenSearch) AuthProviders(ctx context.Context, cid string) (*osmodels.AuthProviders, error) {
	var res *osmodels.AuthProviders
	err := es.operator.ReadOnCluster(ctx, cid, clusters.TypeOpenSearch,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			pillar := ospillars.NewCluster()
			if err = cl.Pillar(pillar); err != nil {
				return err
			}

			res, err = pillar.AuthProviders()

			return err
		},
		clusterslogic.WithPermission(osmodels.PermClustersGet),
	)
	return res, err
}

func names(providers ...*osmodels.AuthProvider) []string {
	res := make([]string, 0, osmodels.MaxAuthProviders)
	for _, p := range providers {
		res = append(res, p.Name)
	}
	return res
}

func (es *OpenSearch) AddAuthProviders(ctx context.Context, cid string, providers ...*osmodels.AuthProvider) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeOpenSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := ospillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			ap, err := pillar.AuthProviders()
			if err != nil {
				return operations.Operation{}, err
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
				osmodels.OperationTypeAddAuthProviders, osmodels.MetadataAddAuthProviders{
					Names: names(providers...),
				})
		},
		clusterslogic.WithPermission(osmodels.PermClustersUpdate),
	)
}

// run task for apply cluster changes
func (es *OpenSearch) applyChanges(
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
			TaskType:      osmodels.TaskTypeClusterModify,
			OperationType: optype,
			Revision:      cluster.Revision,
			Metadata:      metadata,
			Timeout:       optional.NewDuration(updateClusterDefaultTimeout),
			TaskArgs: map[string]interface{}{
				"restart": true,
			},
		})
}

func (es *OpenSearch) UpdateAuthProviders(ctx context.Context, cid string, providers ...*osmodels.AuthProvider) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeOpenSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := ospillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			ap := osmodels.NewAuthProviders()
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
				osmodels.OperationTypeUpdateAuthProviders, osmodels.MetadataUpdateAuthProviders{
					Names: names(providers...),
				})
		},
		clusterslogic.WithPermission(osmodels.PermClustersUpdate),
	)
}

func (es *OpenSearch) DeleteAuthProviders(ctx context.Context, cid string, names ...string) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeOpenSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := ospillars.NewCluster()
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
				osmodels.OperationTypeDeleteAuthProviders, osmodels.MetadataDeleteAuthProviders{
					Names: names,
				})
		},
		clusterslogic.WithPermission(osmodels.PermClustersUpdate),
	)
}

func (es *OpenSearch) UpdateAuthProvider(ctx context.Context, cid string, name string, provider *osmodels.AuthProvider) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeOpenSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := ospillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
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
				osmodels.OperationTypeUpdateAuthProviders, osmodels.MetadataUpdateAuthProviders{
					Names: names(provider),
				})
		},
		clusterslogic.WithPermission(osmodels.PermClustersUpdate),
	)
}
