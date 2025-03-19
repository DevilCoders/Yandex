package provider

import (
	"context"
	"sort"

	cheventspub "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events/mdb/clickhouse"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	validators "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/provider/internal"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (ch *ClickHouse) MLModel(ctx context.Context, cid, name string) (chmodels.MLModel, error) {
	var mlModel chmodels.MLModel
	if err := ch.operator.ReadOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Cluster) error {
			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return err
			}

			for modelName, model := range sc.Pillar.Data.ClickHouse.MLModels {
				if modelName == name {
					mlModel.ClusterID = cid
					mlModel.Name = name
					mlModel.Type = model.Type
					mlModel.URI = model.URI
					return nil
				}
			}

			return semerr.NotFoundf("ML model %q not found", name)
		},
	); err != nil {
		return chmodels.MLModel{}, err
	}

	return mlModel, nil
}

func (ch *ClickHouse) MLModels(ctx context.Context, cid string, pageSize, offset int64) ([]chmodels.MLModel, pagination.OffsetPageToken, error) {
	var mlModels []chmodels.MLModel
	if err := ch.operator.ReadOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Cluster) error {
			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return err
			}

			mlModels = make([]chmodels.MLModel, 0, len(sc.Pillar.Data.ClickHouse.MLModels))
			for modelName, model := range sc.Pillar.Data.ClickHouse.MLModels {
				mlModels = append(mlModels, chmodels.MLModel{
					ClusterID: cid,
					Name:      modelName,
					Type:      model.Type,
					URI:       model.URI,
				})
			}

			sort.Slice(mlModels, func(i, j int) bool {
				return mlModels[i].Name < mlModels[j].Name
			})

			return nil
		},
	); err != nil {
		return nil, pagination.OffsetPageToken{}, err
	}

	page := pagination.NewPage(int64(len(mlModels)), pageSize, offset)

	return mlModels[page.LowerIndex:page.UpperIndex], pagination.OffsetPageToken{
		Offset: page.NextPageOffset,
		More:   page.HasMore,
	}, nil
}

func (ch *ClickHouse) CreateMLModel(ctx context.Context, cid string, name string, modelType chmodels.MLModelType, uri string) (operations.Operation, error) {
	if err := chmodels.MLModelNameValidator.ValidateString(name); err != nil {
		return operations.Operation{}, err
	}

	return ch.operator.CreateOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			if err := ch.validateMlModelURI(ctx, uri); err != nil {
				return operations.Operation{}, err
			}

			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			if err = sc.Pillar.AddMLModel(name, modelType, uri); err != nil {
				return operations.Operation{}, err
			}

			if err = modifier.UpdateSubClusterPillar(ctx, sc.ClusterID, sc.SubClusterID, cluster.Revision, sc.Pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ch.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      chmodels.TaskTypeMLModelCreate,
					OperationType: chmodels.OperationTypeMLModelCreate,
					Metadata:      chmodels.MetadataCreateMLModel{MLModelName: name},
					Revision:      cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target-model": name,
					},
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			event := &cheventspub.CreateMlModel{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.CreateMlModel_STARTED,
				Details: &cheventspub.CreateMlModel_EventDetails{
					ClusterId:   cid,
					MlModelName: name,
				},
			}

			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = ch.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		},
	)
}

func (ch *ClickHouse) UpdateMLModel(ctx context.Context, cid string, name string, uri string) (operations.Operation, error) {
	return ch.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			if err := ch.validateMlModelURI(ctx, uri); err != nil {
				return operations.Operation{}, err
			}

			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			if err = sc.Pillar.UpdateMLModel(name, uri); err != nil {
				return operations.Operation{}, err
			}

			if err = modifier.UpdateSubClusterPillar(ctx, sc.ClusterID, sc.SubClusterID, cluster.Revision, sc.Pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ch.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      chmodels.TaskTypeMLModelModify,
					OperationType: chmodels.OperationTypeMLModelModify,
					Metadata:      chmodels.MetadataUpdateMLModel{MLModelName: name},
					Revision:      cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target-model": name,
					},
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			event := &cheventspub.UpdateMlModel{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.UpdateMlModel_STARTED,
				Details: &cheventspub.UpdateMlModel_EventDetails{
					ClusterId:   cid,
					MlModelName: name,
				},
			}

			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = ch.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		},
	)
}

func (ch *ClickHouse) DeleteMLModel(ctx context.Context, cid string, name string) (operations.Operation, error) {
	return ch.operator.DeleteOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			if err = sc.Pillar.DeleteMLModel(name); err != nil {
				return operations.Operation{}, err
			}

			if err = modifier.UpdateSubClusterPillar(ctx, sc.ClusterID, sc.SubClusterID, cluster.Revision, sc.Pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ch.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      chmodels.TaskTypeMLModelDelete,
					OperationType: chmodels.OperationTypeMLModelDelete,
					Metadata:      chmodels.MetadataDeleteMLModel{MLModelName: name},
					Revision:      cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target-model": name,
					},
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			event := &cheventspub.DeleteMlModel{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.DeleteMlModel_STARTED,
				Details: &cheventspub.DeleteMlModel_EventDetails{
					ClusterId:   cid,
					MlModelName: name,
				},
			}

			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = ch.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		},
	)
}

func (ch *ClickHouse) validateMlModelURI(ctx context.Context, uri string) error {
	return validators.MustExternalResourceURIValidator(ctx, ch.uriValidator, "uri",
		ch.cfg.ClickHouse.ExternalURIValidation.Regexp, ch.cfg.ClickHouse.ExternalURIValidation.Message).ValidateString(uri)
}
