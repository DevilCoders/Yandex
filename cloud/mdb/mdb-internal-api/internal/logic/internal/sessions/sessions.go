package sessions

import (
	"context"

	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/featureflags"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/quota"
)

//go:generate ../../../../../scripts/mockgen.sh Sessions

type SessionOption func(opt interface{})

func WithPrimary() SessionOption {
	return func(opt interface{}) {
		if v, ok := opt.(*sqlutil.NodeStateCriteria); ok {
			*v = sqlutil.Primary
		}
	}
}

func WithPreferStandby() SessionOption {
	return func(opt interface{}) {
		if v, ok := opt.(*sqlutil.NodeStateCriteria); ok {
			*v = sqlutil.PreferStandby
		}
	}
}

type Sessions interface {
	Begin(ctx context.Context, resolver SessionResolver, opts ...SessionOption) (context.Context, Session, error)
	BeginWithIdempotence(ctx context.Context, resolver SessionResolver, opts ...SessionOption) (context.Context, Session, OriginalRequest, error)
	Commit(ctx context.Context) error
	Rollback(ctx context.Context)
}

// Session holds information about request that is being handled
type Session struct {
	// FolderCoords with IDs of folder and cloud the request is aimed at
	FolderCoords    metadb.FolderCoords
	DstFolderCoords *metadb.FolderCoords
	// Subject with authentication info
	Subject as.Subject
	// FeatureFlags from MetaDB and permissions from IAM
	FeatureFlags featureflags.FeatureFlags
	Quota        *quota.Consumption
	DstQuota     *quota.Consumption
	auth         auth.Authenticator
}

func NewSession(folderCoords metadb.FolderCoords, subject as.Subject, featureFlags featureflags.FeatureFlags, quota *quota.Consumption, auth auth.Authenticator, dstFolderCoords *metadb.FolderCoords, dstQuota *quota.Consumption) Session {
	return Session{
		FolderCoords:    folderCoords,
		DstFolderCoords: dstFolderCoords,
		Subject:         subject,
		FeatureFlags:    featureFlags,
		Quota:           quota,
		DstQuota:        dstQuota,
		auth:            auth,
	}
}

func (s *Session) Authorize(ctx context.Context, permission models.Permission, resources ...as.Resource) error {
	subj, err := s.Subject.ToGRPC()
	if err != nil {
		return err
	}
	return s.auth.Authorize(ctx, subj, permission, resources...)
}

func (s Session) ValidateServiceAccount(ctx context.Context, serviceAccountID string) error {
	if serviceAccountID == "" {
		return nil
	}

	if _, err := s.auth.Authenticate(ctx, models.PermIAMServiceAccountsUse, as.ResourceServiceAccount(serviceAccountID)); err != nil {
		return semerr.Authorization("you do not have permission to access the requested service account or service account does not exist")
	}
	return nil
}

// OriginalRequest for the idempotent operation
type OriginalRequest struct {
	// Op created for the original request
	Op operations.Operation
	// Exists is true if there was the same idempotent request before
	Exists bool
}

type SessionResolver struct {
	Target        auth.Target
	ErrorResolver func(error) error
}

func (sr SessionResolver) Validate() error {
	return sr.Target.Validate()
}

// ResolveByCluster construct SessionResolver that treat not found cluster as NotFound error
// useful in ClusterService
func ResolveByCluster(clusterID string, perms models.Permission) SessionResolver {
	return SessionResolver{
		Target:        auth.ByClusterID(clusterID, perms, models.VisibilityVisible),
		ErrorResolver: errorAsIs,
	}
}

// ResolveByBackupID construct SessionResolver that treat not found backup or cluster as NotFound error
// useful in BackupService
func ResolveByBackupID(backupID string, perms models.Permission) SessionResolver {
	return SessionResolver{
		Target:        auth.ByBackupID(backupID, perms, models.VisibilityVisible),
		ErrorResolver: errorAsIs,
	}
}

// ResolveByDeletedCluster construct SessionResolver that purged cluster as NotFound error,
// deleted cluster treated as normal. Useful in restore api
func ResolveByDeletedCluster(clusterID string, perms models.Permission) SessionResolver {
	return SessionResolver{
		Target:        auth.ByClusterID(clusterID, perms, models.VisibilityVisibleOrDeleted),
		ErrorResolver: errorAsIs,
	}
}

// ResolveByClusterDependency construct SessionResolver that treat not found cluster as FailedPrecondition error
// useful in 'depended' services - UserService, DatabaseService...
func ResolveByClusterDependency(clusterID string, perms models.Permission) SessionResolver {
	return SessionResolver{
		Target:        auth.ByClusterID(clusterID, perms, models.VisibilityVisible),
		ErrorResolver: notFoundAsPreconditionFailed,
	}
}

// ResolveByFolder construct SessionResolver for Folder
func ResolveByFolder(folderExtID string, perms models.Permission) SessionResolver {
	return SessionResolver{
		Target:        auth.ByFolderExtID(folderExtID, perms),
		ErrorResolver: errorAsIs,
	}
}

// ResolveByCloud construct SessionResolver for Cloud
func ResolveByCloud(cloudExtID string, perms models.Permission) SessionResolver {
	return SessionResolver{
		Target:        auth.ByCloudExtID(cloudExtID, perms),
		ErrorResolver: errorAsIs,
	}
}

// ResolveByOperation construct SessionResolver for Cloud
func ResolveByOperation(opID string) SessionResolver {
	return SessionResolver{
		Target:        auth.ByOperationID(opID, models.PermMDBAllRead),
		ErrorResolver: errorAsIs,
	}
}

// ResolveByClusterAndDstFolder construct SessionResolver for Cloud
func ResolveByClusterAndDstFolder(clusterID, folderExtID string) SessionResolver {
	return SessionResolver{
		Target: auth.ByClusterIDAndDestinationFolderExtID(
			clusterID,
			folderExtID,
			models.PermMDBAllModify,
			models.VisibilityVisible,
		),
		ErrorResolver: errorAsIs,
	}
}

func notFoundAsPreconditionFailed(err error) error {
	target := semerr.AsSemanticError(err)
	if target != nil {
		if target.Semantic == semerr.SemanticNotFound {
			return semerr.FailedPrecondition(target.Message)
		}
	}
	return err
}

func errorAsIs(err error) error {
	return err
}
