package tags

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Source string

const MetaDBSource Source = "metadb"

// HostMeta is tags for host from metadb
type HostMeta struct {
	ShardID      string   `json:"shard_id,omitempty"`
	SubClusterID string   `json:"subcluster_id,omitempty"`
	Roles        []string `json:"roles,omitempty"`
}

// HostTags are host tags
type HostTags struct {
	Geo  string   `json:"geo,omitempty"`
	Meta HostMeta `json:"meta,omitempty"`
}

func (t HostTags) Marshal() (string, error) {
	bytes, err := json.Marshal(t)
	if err != nil {
		return "", err
	}
	return string(bytes), nil
}

func UnmarshalHostTags(tags string) (HostTags, error) {
	var ret HostTags
	if err := json.Unmarshal([]byte(tags), &ret); err != nil {
		return HostTags{}, xerrors.Errorf("malformed tags: %w", err)
	}
	return ret, nil
}

type ClusterMetaTags struct {
	Type   metadb.ClusterType `json:"type,omitempty"`
	Env    string             `json:"env,omitempty"`
	Rev    int64              `json:"rev,omitempty"`
	Status string             `json:"status,omitempty"`
}

// ClusterTags are cluster tags
type ClusterTags struct {
	Version int             `json:"version"`
	Source  Source          `json:"source"`
	Meta    ClusterMetaTags `json:"meta,omitempty"`
}

func (t ClusterTags) Marshal() (string, error) {
	bytes, err := json.Marshal(t)
	if err != nil {
		return "", err
	}
	return string(bytes), nil
}

func UnmarshalClusterTags(tags string) (ClusterTags, error) {
	var ret ClusterTags
	if err := json.Unmarshal([]byte(tags), &ret); err != nil {
		return ClusterTags{}, xerrors.Errorf("malformed tags: %w", err)
	}
	return ret, nil
}

func QueryTagsBySource(source Source) string {
	q := struct {
		Source string `json:"source"`
	}{Source: string(source)}
	bytes, err := json.Marshal(q)
	if err != nil {
		panic(err)
	}
	return string(bytes)
}
