package network

import "time"

type Subnet struct {
	ID           string
	FolderID     string
	CreatedAt    time.Time
	Name         string
	Description  string
	Labels       map[string]string
	NetworkID    string
	ZoneID       string
	V4CIDRBlocks []string
	V6CIDRBlocks []string
}
