package main

type ResourcePathRecord struct {
	ResourceType string `json:"resource_type"`
	ResourceID   string `json:"resource_id"`
}

type SearchTopicRecord struct {
	ResourceType     string               `json:"resource_type"`
	Timestamp        string               `json:"timestamp"`
	ResourceID       string               `json:"resource_id"`
	Name             string               `json:"name"`
	Service          string               `json:"service"`
	Deleted          string               `json:"deleted,omitempty"`
	Permission       string               `json:"permission"`
	CloudID          string               `json:"cloud_id"`
	FolderID         string               `json:"folder_id"`
	ResourcePath     []ResourcePathRecord `json:"resource_path"`
	ReindexTimestamp string               `json:"reindex_timestamp,omitempty"`
}
