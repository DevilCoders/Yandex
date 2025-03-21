package postgres

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"

	"github.com/jackc/pgconn"
	"github.com/jackc/pgx/v4"

	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/domain"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/store"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/store/pgxscan"
)

func wrapError(err error, format string, args ...interface{}) error {
	if err == nil {
		return err
	}
	var pgErr *pgconn.PgError
	if errors.Is(err, pgx.ErrNoRows) {
		return store.NoRowsErr.WithErr(err, fmt.Sprintf(format, args...))
	}
	if errors.As(err, &pgErr) {
		if pgErr.Code == "23505" {
			return store.ConflictErr.WithErr(err, fmt.Sprintf(format, args...))
		}
	}
	return store.UnknownErr.WithErr(err, fmt.Sprintf(format, args...))
}

type searchData struct {
	Types  []string
	Fields []string
}

// Store DB access layer
type Store struct {
	db *DB
}

func (r *Store) Close() {
	r.db.Close()
}

func (r *Store) CreateNamespace(ctx context.Context, ns domain.Namespace) (domain.Namespace, error) {
	newNamespace := domain.Namespace{}
	err := pgxscan.Get(ctx, r.db, &newNamespace, namespaceInsertQuery, ns.ID, ns.Format, ns.Compatibility, ns.Description, ns.FolderID, ns.Author)
	return newNamespace, wrapError(err, ns.ID)
}

func (r *Store) UpdateNamespace(ctx context.Context, ns domain.Namespace) (domain.Namespace, error) {
	newNamespace := domain.Namespace{}
	err := pgxscan.Get(ctx, r.db, &newNamespace, namespaceUpdateQuery, ns.ID, ns.Format, ns.Compatibility, ns.Description)
	return newNamespace, wrapError(err, ns.ID)
}

func (r *Store) GetNamespace(ctx context.Context, id string) (domain.Namespace, error) {
	newNamespace := domain.Namespace{}
	err := pgxscan.Get(ctx, r.db, &newNamespace, namespaceGetQuery, id)
	return newNamespace, wrapError(err, id)
}

func (r *Store) DeleteNamespace(ctx context.Context, id string) error {
	_, err := r.db.Exec(ctx, namespaceDeleteQuery, id)
	_, _ = r.db.Exec(ctx, deleteOrphanedData)
	return wrapError(err, id)
}

func (r *Store) ListNamespaces(ctx context.Context, folderID string) ([]string, error) {
	var namespaces []string
	err := pgxscan.Select(ctx, r.db, &namespaces, namespaceListQuery, folderID)
	return namespaces, wrapError(err, "")
}

func jsonify(d interface{}) string {
	dd, _ := json.Marshal(d)
	return string(dd)
}

func (r *Store) CreateSchema(ctx context.Context, namespace string, schemaName string, metadata *domain.Metadata, versionID string, file *domain.SchemaFile) (int32, error) {
	var version int32
	err := r.db.BeginFunc(ctx, func(t pgx.Tx) error {
		vErr := t.QueryRow(ctx, getSchemaVersionByID, versionID).Scan(&version)
		if vErr == nil {
			return nil
		}
		if !errors.Is(vErr, pgx.ErrNoRows) {
			return vErr
		}
		var schemaID int32
		if err := t.QueryRow(ctx, schemaInsertQuery, schemaName, namespace, metadata.Format, metadata.Compatibility).Scan(&schemaID); err != nil {
			return err
		}
		if err := t.QueryRow(ctx, versionInsertQuery, schemaID, versionID, file.ID,
			jsonify(&searchData{Types: file.Types, Fields: file.Fields}), file.Data).Scan(&version); err != nil {
			return err
		}
		return nil
	})
	return version, wrapError(err, "create schema failed for %s under%s", schemaName, namespace)
}

func (r *Store) GetSchema(ctx context.Context, namespaceID, schemaName string, versionNumber int32) ([]byte, error) {
	var versionID string
	var data []byte
	if err := r.db.QueryRow(ctx, getVersionIDFromSchemaNameQuery, namespaceID, schemaName, versionNumber).Scan(&versionID); err != nil {
		return nil, wrapError(err, "Get schema for %s - %s", namespaceID, schemaName)
	}
	err := r.db.QueryRow(ctx, getSchemaDataByVersionID, versionID).Scan(&data)
	return data, wrapError(err, "Get schema for %s - %s", namespaceID, schemaName)
}

func (r *Store) GetLatestVersion(ctx context.Context, namespaceID, schemaName string) (int32, error) {
	var version int32
	if err := r.db.QueryRow(ctx, getLatestVersionIDFromSchemaNameQuery, namespaceID, schemaName).Scan(&version); err != nil {
		return version, wrapError(err, "Latest version for %s - %s", namespaceID, schemaName)
	}
	return version, nil
}

func (r *Store) GetSchemaMetadata(ctx context.Context, namespace, sc string) (*domain.Metadata, error) {
	var meta domain.Metadata
	err := pgxscan.Get(ctx, r.db, &meta, getSchemaMetaQuery, namespace, sc)
	return &meta, wrapError(err, "meta")
}

func (r *Store) UpdateSchemaMetadata(ctx context.Context, namespace, sc string, in *domain.Metadata) (*domain.Metadata, error) {
	var meta domain.Metadata
	err := pgxscan.Get(ctx, r.db, &meta, updateSchemaMetaQuery, namespace, sc, in.Compatibility)
	return &meta, wrapError(err, "meta")
}

func (r *Store) ListSchemas(ctx context.Context, namespaceID string) ([]string, error) {
	var schemas []string
	err := pgxscan.Select(ctx, r.db, &schemas, schemaListQuery, namespaceID)
	return schemas, wrapError(err, "List schemas")
}

func (r *Store) DeleteSchema(ctx context.Context, ns string, sc string) error {
	_, err := r.db.Exec(ctx, deleteSchemaQuery, ns, sc)
	// Idempotent operation to clean orphaned data.
	_, _ = r.db.Exec(ctx, deleteOrphanedData)
	return wrapError(err, "delete schema")
}

func (r *Store) ListVersions(ctx context.Context, ns string, sc string) ([]int32, error) {
	var versions []int32
	err := pgxscan.Select(ctx, r.db, &versions, listVersionsQuery, ns, sc)
	return versions, wrapError(err, "versions")
}

func (r *Store) DeleteVersion(ctx context.Context, ns string, sc string, version int32) error {
	_, err := r.db.Exec(ctx, deleteVersionQuery, ns, sc, version)
	// Idempotent operation to clean orphaned data.
	_, _ = r.db.Exec(ctx, deleteOrphanedData)
	return wrapError(err, "delete version")
}

func (r *Store) Search(ctx context.Context, req *domain.SearchRequest) ([]*domain.SearchHits, error) {
	var searchHits []*domain.SearchHits
	err := pgxscan.Select(ctx, r.db, &searchHits, searchAllQuery, req.NamespaceID, req.SchemaID, req.VersionID, req.Query)
	return searchHits, err
}

func (r *Store) SearchLatest(ctx context.Context, req *domain.SearchRequest) ([]*domain.SearchHits, error) {
	var searchHits []*domain.SearchHits
	err := pgxscan.Select(ctx, r.db, &searchHits, searchLatestQuery, req.NamespaceID, req.SchemaID, req.Query)
	return searchHits, err
}
