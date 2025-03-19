package network

import "time"

type Network struct {
	ID           string
	FolderID     string
	RegionID     string
	CreatedAt    time.Time
	Name         string
	Description  string
	Labels       map[string]string
	V4CIDRBlocks []string
	V6CIDRBlocks []string
}
