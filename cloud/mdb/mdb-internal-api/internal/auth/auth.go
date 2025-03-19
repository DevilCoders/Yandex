package auth

import (
	"context"
	"strings"

	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../scripts/mockgen.sh Authenticator

// Authenticator allows for authentication via various means
type Authenticator interface {
	// Authenticate against provided target using standard context token
	// Authenticate (should be = check identity)
	Authenticate(ctx context.Context, perms models.Permission, resources ...as.Resource) (as.Subject, error)
	// Authorize = check permission
	Authorize(ctx context.Context, subject cloudauth.Subject, permission models.Permission, resources ...as.Resource) error
}

// Target to check permissions for
// TODO: this is not used in authenticator anymore and must be refactored
type Target struct {
	Type        TargetType
	Value       string
	Permissions models.Permission
	Visibility  models.Visibility
}

func (t Target) Validate() error {
	if t.Value == "" {
		return xerrors.Errorf("empty auth target value for target %+v", t)
	}

	return nil
}

// TargetType ...
type TargetType int

const (
	// TargetTypeClusterID ...
	TargetTypeClusterID TargetType = iota + 1
	// TargetTypeFolderExtID ...
	TargetTypeFolderExtID
	// TargetTypeCloudExtID ...
	TargetTypeCloudExtID
	// TargetTypeOperationID ...
	TargetTypeOperationID
	// TargetTypeBackupID ...
	TargetTypeBackupID
	// TargetTypeClusterIDAndDestinationFolderID ...
	TargetTypeClusterIDAndDestinationFolderID
)

// ByClusterID constructs target of cluster id type
func ByClusterID(value string, perms models.Permission, vis models.Visibility) Target {
	return Target{Type: TargetTypeClusterID, Value: value, Permissions: perms, Visibility: vis}
}

// ByFolderExtID constructs target of folder ext id type
func ByFolderExtID(value string, perms models.Permission) Target {
	return Target{Type: TargetTypeFolderExtID, Value: value, Permissions: perms}
}

// ByCloudExtID constructs target of cloud ext id type
func ByCloudExtID(value string, perms models.Permission) Target {
	return Target{Type: TargetTypeCloudExtID, Value: value, Permissions: perms}
}

// ByOperationID constructs target of operation id type
func ByOperationID(value string, perms models.Permission) Target {
	return Target{Type: TargetTypeOperationID, Value: value, Permissions: perms}
}

// ByBackupID constructs target of backup id type
func ByBackupID(value string, perms models.Permission, vis models.Visibility) Target {
	return Target{Type: TargetTypeBackupID, Value: value, Permissions: perms, Visibility: vis}
}

// ByClusterIDAndDestinationFolderExtID constructs target of cluster id and destination folder id type
func ByClusterIDAndDestinationFolderExtID(clusterID, folderExtID string, perms models.Permission, vis models.Visibility) Target {
	return Target{
		Type:        TargetTypeClusterIDAndDestinationFolderID,
		Value:       clusterIDAndFolderExtIDToTargetValue(clusterID, folderExtID),
		Permissions: perms,
		Visibility:  vis,
	}
}

func clusterIDAndFolderExtIDToTargetValue(clusterID, folderExtID string) string {
	return clusterID + "=>" + folderExtID
}

func (t *Target) GetClusterIDAndFolderExtID() (string, string, error) {
	if t.Type != TargetTypeClusterIDAndDestinationFolderID {
		return "", "", xerrors.Errorf("target value of target with type %v can not be unpacked to cluster id and folder id", t.Type)
	}

	parts := strings.Split(t.Value, "=>")
	if len(parts) != 2 {
		return "", "", xerrors.Errorf("target value can not be unpacked to cluster id and folder id")
	}

	return parts[0], parts[1], nil
}
