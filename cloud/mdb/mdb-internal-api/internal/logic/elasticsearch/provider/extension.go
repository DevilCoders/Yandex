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

func (es *ElasticSearch) Extension(ctx context.Context, cid string, extensionID string) (esmodels.Extension, error) {
	var res esmodels.Extension
	err := es.operator.ReadOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			pillar := espillars.NewCluster()
			if err = cl.Pillar(pillar); err != nil {
				return err
			}

			extensions := pillar.Extensions()

			res, err = extensions.FindByID(extensionID)

			return err
		},
		clusterslogic.WithPermission(esmodels.PermClustersGet),
	)
	return res, err
}

func (es *ElasticSearch) Extensions(ctx context.Context, cid string) ([]esmodels.Extension, error) {
	var res []esmodels.Extension
	err := es.operator.ReadOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			pillar := espillars.NewCluster()
			if err = cl.Pillar(pillar); err != nil {
				return err
			}

			extensions := pillar.Extensions()
			res = extensions.GetList()

			return nil
		},
		clusterslogic.WithPermission(esmodels.PermClustersGet),
	)
	return res, err
}

func (es *ElasticSearch) CreateExtension(ctx context.Context, cid, name, uri string, disabled bool) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {

			if err := es.ValidateExtensionURI(name, uri); err != nil {
				return operations.Operation{}, err
			}

			id, err := es.extIDGen.Generate()
			if err != nil {
				return operations.Operation{}, err
			}

			pillar := espillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			exts := pillar.Extensions()

			if err := exts.Add(id, name, uri, disabled); err != nil {
				return operations.Operation{}, err
			}

			pillar.SetExtensions(exts)

			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			return es.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cluster.ClusterID,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      esmodels.TaskTypeClusterModify,
					OperationType: esmodels.OperationTypeCreateExtension,
					Revision:      cluster.Revision,
					Metadata: esmodels.MetadataCreateExtension{
						ExtensionID: id,
					},
					Timeout: optional.NewDuration(updateClusterDefaultTimeout),
					TaskArgs: map[string]interface{}{
						"extensions": []string{id},
						// "restart":    true,
					},
				},
			)
		},
		clusterslogic.WithPermission(esmodels.PermClustersUpdate),
	)
}

func (es *ElasticSearch) DeleteExtension(ctx context.Context, cid, extensionID string) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := espillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			exts := pillar.Extensions()

			if err := exts.Delete(extensionID); err != nil {
				return operations.Operation{}, err
			}

			pillar.SetExtensions(exts)

			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			return es.applyChanges(ctx, session, cluster,
				esmodels.OperationTypeDeleteExtension, esmodels.MetadataDeleteExtension{
					ExtensionID: extensionID,
				},
			)
		},
		clusterslogic.WithPermission(esmodels.PermClustersUpdate),
	)
}

func (es *ElasticSearch) UpdateExtension(ctx context.Context, cid, extensionID string, active bool) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := espillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			exts := pillar.Extensions()

			if err := exts.Update(extensionID, active); err != nil {
				return operations.Operation{}, err
			}

			pillar.SetExtensions(exts)

			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			return es.applyChanges(ctx, session, cluster,
				esmodels.OperationTypeUpdateExtension, esmodels.MetadataUpdateExtension{
					ExtensionID: extensionID,
				},
			)
		},
		clusterslogic.WithPermission(esmodels.PermClustersUpdate),
	)
}

func (es *ElasticSearch) ValidateExtensionURI(name, uri string) error {
	if err := esmodels.ValidateExtensionURI(uri, es.cfg.Elasticsearch.ExtensionAllowedDomain); err != nil {
		return semerr.InvalidInputf("invalid uri for extension %q: %s", name, err)
	}
	if es.cfg.Elasticsearch.ExtensionPrevalidation {
		if err := esmodels.ValidateExtensionArchive(uri); err != nil {
			return semerr.InvalidInputf("invalid uri for extension %q: %s", name, err)
		}
	}
	return nil
}
