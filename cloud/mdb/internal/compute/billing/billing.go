package billing

import (
	"context"
	"encoding/json"
)

// ComputeInstance ...
type ComputeInstance struct {
	FolderID   string
	PlatformID string
	ZoneID     *string
	Resources  ComputeInstanceResources
	SubnetID   *string
	BootDisk   ComputeDisk
}

// ComputeInstanceResources ...
type ComputeInstanceResources struct {
	Memory       uint64
	Cores        int
	CoreFraction int
	GPUs         int
}

// ComputeDisk ...
type ComputeDisk struct {
	TypeID  *string
	Size    *uint64
	ImageID string
}

// Metric ...
type Metric struct {
	FolderID string
	Schema   string
	Tags     map[string]json.RawMessage
}

// Client defines client to compute billing
type Client interface {
	Metrics(ctx context.Context, instance ComputeInstance) ([]Metric, error)
}
