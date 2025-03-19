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

func (ch *ClickHouse) FormatSchema(ctx context.Context, cid, name string) (chmodels.FormatSchema, error) {
	var formatSchema chmodels.FormatSchema
	if err := ch.operator.ReadOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Cluster) error {
			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return err
			}

			for schemaName, schema := range sc.Pillar.Data.ClickHouse.FormatSchemas {
				if schemaName == name {
					formatSchema.ClusterID = cid
					formatSchema.Name = name
					formatSchema.Type = chmodels.FormatSchemaType(schema.Type)
					formatSchema.URI = schema.URI
					return nil
				}
			}

			return semerr.NotFoundf("format schema %q not found", name)
		},
	); err != nil {
		return chmodels.FormatSchema{}, err
	}

	return formatSchema, nil
}

func (ch *ClickHouse) FormatSchemas(ctx context.Context, cid string, pageSize, offset int64) ([]chmodels.FormatSchema, pagination.OffsetPageToken, error) {
	var formatSchemas []chmodels.FormatSchema
	if err := ch.operator.ReadOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Cluster) error {
			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return err
			}

			formatSchemas = make([]chmodels.FormatSchema, 0, len(sc.Pillar.Data.ClickHouse.FormatSchemas))
			for schemaName, schema := range sc.Pillar.Data.ClickHouse.FormatSchemas {
				formatSchemas = append(formatSchemas, chmodels.FormatSchema{
					ClusterID: cid,
					Name:      schemaName,
					Type:      schema.Type,
					URI:       schema.URI,
				})
			}

			sort.Slice(formatSchemas, func(i, j int) bool {
				return formatSchemas[i].Name < formatSchemas[j].Name
			})

			return nil
		},
	); err != nil {
		return nil, pagination.OffsetPageToken{}, err
	}

	page := pagination.NewPage(int64(len(formatSchemas)), pageSize, offset)

	return formatSchemas[page.LowerIndex:page.UpperIndex], pagination.OffsetPageToken{
		Offset: page.NextPageOffset,
		More:   page.HasMore,
	}, nil
}

func (ch *ClickHouse) CreateFormatSchema(ctx context.Context, cid string, name string, formatSchemaType chmodels.FormatSchemaType, uri string) (operations.Operation, error) {
	if err := chmodels.FormatSchemaNameValidator.ValidateString(name); err != nil {
		return operations.Operation{}, err
	}

	return ch.operator.CreateOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			if err := ch.validateFormatSchemaURI(ctx, uri); err != nil {
				return operations.Operation{}, err
			}

			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			if err = sc.Pillar.AddFormatSchema(name, formatSchemaType, uri); err != nil {
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
					TaskType:      chmodels.TaskTypeFormatSchemaCreate,
					OperationType: chmodels.OperationTypeFormatSchemaCreate,
					Metadata:      chmodels.MetadataCreateFormatSchema{FormatSchemaName: name},
					Revision:      cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target-format-schema": name,
					},
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			event := &cheventspub.CreateFormatSchema{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.CreateFormatSchema_STARTED,
				Details: &cheventspub.CreateFormatSchema_EventDetails{
					ClusterId:        cid,
					FormatSchemaName: name,
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

func (ch *ClickHouse) UpdateFormatSchema(ctx context.Context, cid string, name string, uri string) (operations.Operation, error) {
	return ch.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			if err := ch.validateFormatSchemaURI(ctx, uri); err != nil {
				return operations.Operation{}, err
			}

			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			if err = sc.Pillar.UpdateFormatSchema(name, uri); err != nil {
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
					TaskType:      chmodels.TaskTypeFormatSchemaModify,
					OperationType: chmodels.OperationTypeFormatSchemaModify,
					Metadata:      chmodels.MetadataUpdateFormatSchema{FormatSchemaName: name},
					Revision:      cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target-format-schema": name,
					},
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			event := &cheventspub.UpdateFormatSchema{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.UpdateFormatSchema_STARTED,
				Details: &cheventspub.UpdateFormatSchema_EventDetails{
					ClusterId:        cid,
					FormatSchemaName: name,
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

func (ch *ClickHouse) DeleteFormatSchema(ctx context.Context, cid string, name string) (operations.Operation, error) {
	return ch.operator.DeleteOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			if err = sc.Pillar.DeleteFormatSchema(name); err != nil {
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
					TaskType:      chmodels.TaskTypeFormatSchemaDelete,
					OperationType: chmodels.OperationTypeFormatSchemaDelete,
					Metadata:      chmodels.MetadataDeleteFormatSchema{FormatSchemaName: name},
					Revision:      cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target-format-schema": name,
					},
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			event := &cheventspub.DeleteFormatSchema{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.DeleteFormatSchema_STARTED,
				Details: &cheventspub.DeleteFormatSchema_EventDetails{
					ClusterId:        cid,
					FormatSchemaName: name,
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

func (ch *ClickHouse) validateFormatSchemaURI(ctx context.Context, uri string) error {
	return validators.MustExternalResourceURIValidator(ctx, ch.uriValidator, "uri",
		ch.cfg.ClickHouse.ExternalURIValidation.Regexp, ch.cfg.ClickHouse.ExternalURIValidation.Message).ValidateString(uri)
}
