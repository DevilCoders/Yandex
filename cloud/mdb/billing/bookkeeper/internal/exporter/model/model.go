package model

import "encoding/json"

type Metric interface {
	Marshal() ([]byte, error)
}

type MetricUsage struct {
	Type     string `json:"type"`
	Quantity int64  `json:"quantity"`
	Unit     string `json:"unit"`
	Start    int64  `json:"start"`
	Finish   int64  `json:"finish"`
}

type CloudMetric struct {
	CloudID    string            `json:"cloud_id"`
	FolderID   string            `json:"folder_id"`
	ResourceID string            `json:"resource_id"`
	Tags       interface{}       `json:"tags"`
	ID         string            `json:"id"`
	Usage      MetricUsage       `json:"usage"`
	Version    string            `json:"version,omitempty"`
	SourceID   string            `json:"source_id"`
	Schema     string            `json:"schema"`
	Labels     map[string]string `json:"labels"`
	SourceWT   int64             `json:"source_wt"`
}

func (m *CloudMetric) Marshal() ([]byte, error) {
	return json.Marshal(m)
}
