package resmanager

import (
	"context"
)

//go:generate ../../../scripts/mockgen.sh Client

// Client for Resource Manager
type Client interface {
	Cloud(ctx context.Context, cloudExtID string) (Cloud, error)
	PermissionStages(ctx context.Context, cloudExtID string) ([]string, error)
	ResolveFolders(ctx context.Context, folderExtIDs []string) ([]ResolvedFolder, error)
	CheckServiceAccountRole(ctx context.Context, iamToken, folderID, serviceAccountID, roleID string) (bool, error)
	ListAccessBindings(ctx context.Context, resourceID string, private bool) ([]AccessBinding, error)
}

type ResolvedFolder struct {
	FolderExtID string
	// Do not export resource_path for now
	CloudExtID        string
	OrganizationExtID string
}

type Subject struct {
	ID   string
	Type string
}

type AccessBinding struct {
	RoleID  string
	Subject Subject
}

type CloudStatus int32

const (
	CloudStatusUnknown CloudStatus = iota
	CloudStatusCreating
	CloudStatusActive
	CloudStatusDeleting
	CloudStatusBlocked
	CloudStatusPendingDeletion
)

type Cloud struct {
	CloudID string
	Name    string
	Status  CloudStatus
}
