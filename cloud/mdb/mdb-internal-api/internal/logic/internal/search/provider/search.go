package provider

import (
	"context"
	"encoding/json"
	"fmt"
	"sort"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const clusterResource = "cluster"

type resourcePath struct {
	ResourceType string `json:"resource_type"`
	ResourceID   string `json:"resource_id"`
}

// searchDoc is base doc for YC-search
type searchDoc struct {
	Timestamp        string                 `json:"timestamp"`
	ReindexTimestamp string                 `json:"reindex_timestamp,omitempty"`
	Deleted          string                 `json:"deleted,omitempty"`
	ResourceType     string                 `json:"resource_type"`
	ResourceID       string                 `json:"resource_id"`
	ResourcePath     []resourcePath         `json:"resource_path"`
	Service          string                 `json:"service"`
	Permission       string                 `json:"permission"`
	FolderID         string                 `json:"folder_id"`
	CloudID          string                 `json:"cloud_id"`
	Name             string                 `json:"name"`
	Attributes       map[string]interface{} `json:"attributes"`
}

func (d searchDoc) Marshal() ([]byte, error) {
	ret, err := json.Marshal(d)
	if err != nil {
		return nil, fmt.Errorf("search doc marashal: %s", err)
	}
	return ret, nil
}

func fmtTime(t time.Time) string {
	return t.In(time.UTC).Format(time.RFC3339)
}

type Search struct {
	metaDB metadb.Backend
}

func NewSearch(metaDB metadb.Backend) *Search {
	return &Search{metaDB: metaDB}
}

func (d *Search) base(service, folderExtID, cloudExtID, name string, op operations.Operation) searchDoc {
	return searchDoc{
		Timestamp:    fmtTime(op.CreatedAt),
		ResourceType: clusterResource,
		ResourceID:   op.ClusterID,
		ResourcePath: []resourcePath{{
			ResourceID:   folderExtID,
			ResourceType: "resource-manager.folder",
		}},
		Name:       name,
		Service:    service,
		Permission: models.PermMDBAllRead.Name,
		FolderID:   folderExtID,
		CloudID:    cloudExtID,
	}
}

func (d *Search) StoreDoc(ctx context.Context, service, folderExtID, cloudExtID string, operation operations.Operation, attributesExtractor search.AttributesExtractor) error {
	cluster, err := d.metaDB.ClusterByClusterID(ctx, operation.ClusterID, models.VisibilityAll)
	if err != nil {
		return xerrors.Errorf("get cluster for a search doc: %w", err)
	}
	newDoc, err := d.gatherSearchDoc(ctx, service, folderExtID, cloudExtID, operation, attributesExtractor, cluster)
	if err != nil {
		return err
	}

	return d.store(ctx, newDoc)
}

func (d *Search) gatherSearchDoc(ctx context.Context, service string, folderExtID string, cloudExtID string, operation operations.Operation, attributesExtractor search.AttributesExtractor, cluster metadb.Cluster) (searchDoc, error) {
	hosts, _, _, err := d.metaDB.ListHosts(ctx, operation.ClusterID, 0, optional.Int64{})
	if err != nil {
		return searchDoc{}, xerrors.Errorf("get hosts for a search doc: %w", err)
	}
	fqdns := make([]string, len(hosts))
	for i, h := range hosts {
		fqdns[i] = h.FQDN
	}
	sort.Strings(fqdns)

	attributes, err := attributesExtractor(clusters.NewClusterModel(cluster.Cluster, cluster.Pillar))
	if err != nil {
		return searchDoc{}, xerrors.Errorf("build search doc attributes: %w", err)
	}
	attributes["name"] = cluster.Name
	attributes["description"] = cluster.Description
	attributes["labels"] = cluster.Labels
	attributes["hosts"] = fqdns

	newDoc := d.base(service, folderExtID, cloudExtID, cluster.Name, operation)
	newDoc.Attributes = attributes
	return newDoc, nil
}

func (d *Search) StoreDocReindex(ctx context.Context, service, cid string, attributesExtractor search.AttributesExtractor) error {
	coords, rev, _, err := d.metaDB.FolderCoordsByClusterID(ctx, cid, models.VisibilityVisible)
	if err != nil {
		return xerrors.Errorf("get foolder coords by cluster: %w", err)
	}
	cluster, err := d.metaDB.ClusterByClusterIDAtRevision(ctx, cid, rev)
	if err != nil {
		return xerrors.Errorf("get cluster at reindex for a search doc reindex: %w", err)
	}
	op, err := d.metaDB.MostRecentInitiatedByUserOperationByClusterID(ctx, cid)
	if err != nil {
		return xerrors.Errorf("get most recent initiated by user operation by cluster %q: %w", cid, err)
	}

	doc, err := d.gatherSearchDoc(ctx, service, coords.FolderExtID, coords.CloudExtID, op, attributesExtractor, cluster)
	if err != nil {
		return xerrors.Errorf("build reindex doc: %w", err)
	}
	doc.ReindexTimestamp = fmtTime(time.Now())
	return d.store(ctx, doc)
}

func (d *Search) StoreDocDelete(ctx context.Context, service, folderExtID, cloudExtID string, operation operations.Operation) error {
	cluster, err := d.metaDB.ClusterByClusterID(ctx, operation.ClusterID, models.VisibilityAll)
	if err != nil {
		return xerrors.Errorf("get cluster by id: %w", err)
	}
	doc := d.base(service, folderExtID, cloudExtID, cluster.Name, operation)
	doc.Deleted = fmtTime(operation.CreatedAt)
	doc.Attributes = make(map[string]interface{})

	return d.store(ctx, doc)
}

func (d *Search) store(ctx context.Context, doc searchDoc) error {
	b, err := doc.Marshal()
	if err != nil {
		return xerrors.Errorf("marshal search doc: %w", err)
	}
	if err := d.metaDB.CreateSearchQueueDoc(ctx, b); err != nil {
		return xerrors.Errorf("store search doc: %w", err)
	}
	return nil
}
