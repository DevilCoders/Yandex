// TODO: think about another name for Sessions, Session and their methods

package provider

import (
	"bytes"
	"context"

	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	"a.yandex-team.ru/cloud/mdb/internal/featureflags"
	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
	"a.yandex-team.ru/cloud/mdb/internal/request"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/quota"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// notFoundID is a fake ID of folder or cloud that is not exists in DB
const notFoundID int64 = -1

type Sessions struct {
	metaDB            metadb.Backend
	auth              auth.Authenticator
	rm                resmanager.Client
	defaultCloudQuota quota.Resources
	resourceModel     environment.ResourceModel
}

var _ sessions.Sessions = &Sessions{}

func NewSessions(metaDB metadb.Backend, auth auth.Authenticator, rm resmanager.Client, defaultCloudQuota quota.Resources, resModel environment.ResourceModel) *Sessions {
	return &Sessions{metaDB: metaDB, auth: auth, rm: rm, defaultCloudQuota: defaultCloudQuota, resourceModel: resModel}
}

func (s *Sessions) cloudByExtID(ctx context.Context, cloudExtID string, createMissing bool) (metadb.Cloud, error) {
	cloud, err := s.metaDB.CloudByCloudExtID(ctx, cloudExtID)
	// Success
	if err == nil {
		return cloud, nil
	}

	// Error getting info from DB
	if !xerrors.Is(err, sqlerrors.ErrNotFound) {
		return metadb.Cloud{}, xerrors.Errorf("failed to get cloud info from DB %q: %w", cloudExtID, err)
	}

	if !createMissing {
		return metadb.Cloud{
			CloudID:    notFoundID,
			CloudExtID: cloudExtID,
		}, nil
	}

	// Making new cloud
	requestID, ok := requestid.CheckedFromContext(ctx)
	if !ok {
		return metadb.Cloud{}, xerrors.New("failed to get request id from context")
	}
	return s.metaDB.CreateCloud(ctx, cloudExtID, metadb.Resources(s.defaultCloudQuota), requestID)
}

func (s *Sessions) folderCoordsByExtID(ctx context.Context, folderExtID string, createMissing bool) (metadb.FolderCoords, error) {
	fcoords, err := s.metaDB.FolderCoordsByFolderExtID(ctx, folderExtID)
	// Success
	if err == nil {
		return fcoords, nil
	}

	// Error getting info from DB
	if !xerrors.Is(err, sqlerrors.ErrNotFound) {
		return metadb.FolderCoords{}, xerrors.Errorf("folder coords by folder ext id %q: %w", folderExtID, err)
	}

	resolvedFolder, err := resmanager.ResolveFolder(ctx, s.rm, folderExtID)
	if err != nil {
		if xerrors.Is(err, resmanager.ErrFolderNotFound) {
			return metadb.FolderCoords{}, semerr.NotFoundf("%s id %q not found", s.resourceModel.Folder(), folderExtID)
		} else {
			return metadb.FolderCoords{}, err
		}
	}

	// TODO: split this func, move everything below this line to createFolder

	// Folder not found and we should not create it
	if !createMissing {
		return metadb.FolderCoords{
			CloudID:     notFoundID,
			CloudExtID:  resolvedFolder.CloudExtID,
			FolderID:    notFoundID,
			FolderExtID: folderExtID,
		}, nil
	}

	cloud, err := s.cloudByExtID(ctx, resolvedFolder.CloudExtID, true)
	if err != nil {
		return metadb.FolderCoords{}, xerrors.Errorf("cloud by cloud ext id %q: %w", resolvedFolder.CloudExtID, err)
	}

	// Making new folder
	fcoords, err = s.metaDB.CreateFolder(ctx, folderExtID, cloud.CloudID)
	if err != nil {
		return metadb.FolderCoords{}, xerrors.Errorf("create folder ext id %q: %w", folderExtID, err)
	}

	return fcoords, nil
}

func (s *Sessions) createFolder(ctx context.Context, fcoords metadb.FolderCoords, resolver sessions.SessionResolver) (metadb.FolderCoords, error) {
	if resolver.Target.Type != auth.TargetTypeFolderExtID {
		return fcoords, nil
	}

	if !resolver.Target.Permissions.IsCreate() {
		return fcoords, nil
	}

	if fcoords.FolderID != notFoundID {
		return fcoords, nil
	}

	return s.folderCoordsByExtID(ctx, resolver.Target.Value, true)
}

func (s *Sessions) loadFeatureFlags(ctx context.Context, cloudExtID string) (featureflags.FeatureFlags, error) {
	iamPermissions, err := s.rm.PermissionStages(ctx, cloudExtID)
	if err != nil {
		return featureflags.FeatureFlags{}, xerrors.Errorf("failed to retrieve cloud by cloud Ext ID %q: %w", cloudExtID, err)
	}

	var cloudFeatureFlags []string
	cloud, err := s.metaDB.CloudByCloudExtID(ctx, cloudExtID)
	if err != nil {
		if !xerrors.Is(err, sqlerrors.ErrNotFound) {
			return featureflags.FeatureFlags{}, err
		}
		cloudFeatureFlags, err = s.metaDB.DefaultFeatureFlags(ctx)
		if err != nil {
			return featureflags.FeatureFlags{}, xerrors.Errorf("failed to retrieve default feature flags")
		}
	} else {
		cloudFeatureFlags = cloud.FeatureFlags
	}

	// Merge feature flags from MetaDB and permissions from IAM
	ffs := featureflags.NewFeatureFlags(append(cloudFeatureFlags, iamPermissions...))
	return ffs, nil
}

// cleanupTxContext is used to defer rollback of tx stored in context in case of an error
// Usage:
//	defer func() {
//		s.cleanupTxContext(ctxTx, err, err2)
//	}()

func (s *Sessions) cleanupTxContext(ctx context.Context, errs ...error) {
	if r := recover(); r != nil {
		_ = s.metaDB.Rollback(ctx)
		panic(r)
	}

	// Rollback if we have ANY error
	for _, err := range errs {
		if err != nil {
			_ = s.metaDB.Rollback(ctx)
			break
		}
	}
}

func (s *Sessions) prepareAuth(ctx context.Context, ns sqlutil.NodeStateCriteria, resolver sessions.SessionResolver) (context.Context, metadb.FolderCoords, []as.Resource, error) {
	var fcoords metadb.FolderCoords
	var resources []as.Resource
	var err error

	switch resolver.Target.Type {
	case auth.TargetTypeClusterID:
		tags.ClusterID.SetContext(ctx, resolver.Target.Value)
		ctx = ctxlog.WithFields(ctx, log.String("cluster.id", resolver.Target.Value))
		ctx, fcoords, resources, err = s.prepareAuthByClusterID(ctx, ns, resolver)
	case auth.TargetTypeBackupID:
		tags.BackupID.SetContext(ctx, resolver.Target.Value)
		ctx = ctxlog.WithFields(ctx, log.String("backup.id", resolver.Target.Value))
		ctx, fcoords, resources, err = s.prepareAuthByBackupID(ctx, ns, resolver)
	case auth.TargetTypeFolderExtID:
		tags.FolderExtID.SetContext(ctx, resolver.Target.Value)
		ctx = ctxlog.WithFields(ctx, log.String("folder.ext_id", resolver.Target.Value))
		ctx, fcoords, resources, err = s.prepareAuthByFolderExtID(ctx, ns, resolver)
	case auth.TargetTypeCloudExtID:
		tags.CloudExtID.SetContext(ctx, resolver.Target.Value)
		ctx = ctxlog.WithFields(ctx, log.String("cloud.ext_id", resolver.Target.Value))
		ctx, fcoords, resources, err = s.prepareAuthByCloudExtID(ctx, ns, resolver)
	case auth.TargetTypeOperationID:
		tags.OperationID.SetContext(ctx, resolver.Target.Value)
		ctx = ctxlog.WithFields(ctx, log.String("operation.id", resolver.Target.Value))
		ctx, fcoords, resources, err = s.prepareAuthByOperationID(ctx, resolver)
	case auth.TargetTypeClusterIDAndDestinationFolderID:
		var clusterID string
		clusterID, _, err = resolver.Target.GetClusterIDAndFolderExtID()
		if err != nil {
			return ctx, fcoords, resources, err
		}
		tmpResolver := resolver
		tmpResolver.Target.Value = clusterID

		tags.ClusterID.SetContext(ctx, clusterID)
		ctx = ctxlog.WithFields(ctx, log.String("cluster.id", clusterID))
		ctx, fcoords, resources, err = s.prepareAuthByClusterID(ctx, ns, tmpResolver)
	default:
		return ctx, metadb.FolderCoords{}, nil, xerrors.Errorf("invalid authentication target type: %d", resolver.Target.Type)
	}

	if err != nil {
		return ctx, fcoords, resources, err
	}

	if resolver.Target.Type != auth.TargetTypeCloudExtID {
		tags.CloudExtID.SetContext(ctx, fcoords.CloudExtID)
		ctx = ctxlog.WithFields(ctx, log.String("cloud.ext_id", fcoords.CloudExtID))
		request.SetRequestCloudID(ctx, fcoords.CloudExtID)
	}
	if resolver.Target.Type != auth.TargetTypeFolderExtID {
		tags.FolderExtID.SetContext(ctx, fcoords.FolderExtID)
		ctx = ctxlog.WithFields(ctx, log.String("folder.ext_id", fcoords.FolderExtID))
		request.SetRequestFolderID(ctx, fcoords.FolderExtID)
	}

	return ctx, fcoords, resources, err
}

// prepareAuthByClusterID starts two transactions and loads folder coordinates from database.
// Either of the transactions/loads may fail.
// If Primary is requested right away, we use only that node.
// If cluster revision on primary > cluster revision on standby, we use values from primary. Use values from standby otherwise.
// ATTENTION: be ULTRA-careful with error-handling and returned context/error values, any misstep will result in leaked transaction(s)
// TODO: start both transactions and load folder coords in parallel to reduce latency
// TODO: https://clubs.at.yandex-team.ru/ycp/1571 - add cluster id (requires some prior action, check link) to returned resources
func (s *Sessions) prepareAuthByClusterID(ctx context.Context, ns sqlutil.NodeStateCriteria, resolver sessions.SessionResolver) (_ context.Context, _ metadb.FolderCoords, _ []as.Resource, err error) {
	// Alive might end up accessing Primary node so for now we 'downgrade' to Standby.
	// TODO: This code optimizes Primary load but we want to optimize latency. We must use Alive but check if we've got Primary or not.
	if ns != sqlutil.Primary {
		ns = sqlutil.Standby
	}

	var fcoordsDefault metadb.FolderCoords
	var revDefault int64
	var clusterType clusters.Type
	var resourcePathDefault []as.Resource
	ctxTxDefault, errDefault := s.metaDB.Begin(ctx, ns)
	if errDefault == nil {
		defer func() {
			// ATTENTION: MUST capture err (which in turn MUST be named return value) and errors from default db
			s.cleanupTxContext(ctxTxDefault, err, errDefault)
		}()

		fcoordsDefault, revDefault, clusterType, errDefault = s.metaDB.FolderCoordsByClusterID(ctxTxDefault, resolver.Target.Value, resolver.Target.Visibility)
		resourcePathDefault = []as.Resource{
			{Type: clusterType.Info().ResourceName, ID: resolver.Target.Value},
			as.ResourceFolder(fcoordsDefault.FolderExtID),
		}
	}

	if ns == sqlutil.Primary {
		// We went for primary right away, happy path!
		if errDefault != nil {
			if xerrors.Is(errDefault, sqlerrors.ErrNotFound) {
				return ctx, metadb.FolderCoords{}, nil, resolver.ErrorResolver(semerr.NotFoundf("cluster id %q not found", resolver.Target.Value))
			}

			return ctx, metadb.FolderCoords{}, nil, xerrors.Errorf("folder coords for cluster id %q not retrieved: %w", resolver.Target.Value, errDefault)
		}

		return ctxTxDefault, fcoordsDefault, resourcePathDefault, nil
	}

	// Load values from primary
	var fcoordsPrimary metadb.FolderCoords
	var revPrimary int64
	var resourcePathPrimary []as.Resource
	ctxTxPrimary, errPrimary := s.metaDB.Begin(ctx, sqlutil.Primary)
	if errPrimary == nil {
		defer func() {
			// ATTENTION: MUST capture err (which in turn MUST be named return value) and errors from primary db
			s.cleanupTxContext(ctxTxPrimary, err, errPrimary)
		}()

		fcoordsPrimary, revPrimary, clusterType, errPrimary = s.metaDB.FolderCoordsByClusterID(ctxTxPrimary, resolver.Target.Value, resolver.Target.Visibility)
		resourcePathPrimary = []as.Resource{
			{Type: clusterType.Info().ResourceName, ID: resolver.Target.Value},
			as.ResourceFolder(fcoordsPrimary.FolderExtID),
		}
	}

	// Handle error on primary
	if errPrimary != nil {
		// Primary failed
		if xerrors.Is(errPrimary, sqlerrors.ErrNotFound) {
			// Not found on primary
			return ctx, metadb.FolderCoords{}, nil, resolver.ErrorResolver(semerr.NotFoundf("cluster id %q not found", resolver.Target.Value))
		}

		if errDefault != nil {
			// Standby failed too
			if xerrors.Is(errDefault, sqlerrors.ErrNotFound) {
				// Not found on standby, report NotFound even when primary is unavailable
				return ctx, metadb.FolderCoords{}, nil, resolver.ErrorResolver(semerr.NotFoundf("cluster id %q not found", resolver.Target.Value))
			}

			return ctx, metadb.FolderCoords{}, nil, xerrors.Errorf("folder coords for cluster id %q not retrieved: %w", resolver.Target.Value, errPrimary)
		}

		// Return standby values
		return ctxTxDefault, fcoordsDefault, resourcePathDefault, nil
	}

	// We have values from primary
	if errDefault != nil {
		// Return primary values
		return ctxTxPrimary, fcoordsPrimary, resourcePathPrimary, nil
	}

	// We have values from both primary and standby
	if revDefault != revPrimary {
		// Standby has stale data, use primary
		return ctxTxPrimary, fcoordsPrimary, resourcePathPrimary, nil
	}

	// Standby data is the same as primary
	return ctxTxDefault, fcoordsDefault, resourcePathDefault, nil
}

// If we are doing auth by backup id, then find cid of given backup and fallback to auth by CID
func (s *Sessions) prepareAuthByBackupID(ctx context.Context, ns sqlutil.NodeStateCriteria, resolver sessions.SessionResolver) (context.Context, metadb.FolderCoords, []as.Resource, error) {
	cid, err := s.metaDB.ClusterIDByBackupID(ctx, resolver.Target.Value)

	if err != nil {
		if xerrors.Is(err, sqlerrors.ErrNotFound) {
			return ctx, metadb.FolderCoords{}, nil, resolver.ErrorResolver(semerr.NotFoundf("backup id %q not found", resolver.Target.Value))
		}

		return ctx, metadb.FolderCoords{}, nil, xerrors.Errorf("folder coords for backup id %q not retrieved: %w", resolver.Target.Value, err)
	}

	cidResolver := sessions.SessionResolver{
		Target:        auth.ByClusterID(cid, resolver.Target.Permissions, resolver.Target.Visibility),
		ErrorResolver: resolver.ErrorResolver,
	}

	tags.ClusterID.SetContext(ctx, cidResolver.Target.Value)
	ctx = ctxlog.WithFields(ctx, log.String("cluster.id", cidResolver.Target.Value))
	return s.prepareAuthByClusterID(ctx, ns, cidResolver)
}

func (s *Sessions) prepareAuthByFolderExtID(ctx context.Context, ns sqlutil.NodeStateCriteria, resolver sessions.SessionResolver) (context.Context, metadb.FolderCoords, []as.Resource, error) {
	ctxTx, err := s.metaDB.Begin(ctx, ns)
	if err != nil {
		return ctx, metadb.FolderCoords{}, nil, err
	}
	defer func() {
		s.cleanupTxContext(ctxTx, err)
	}()

	fcoords, err := s.folderCoordsByExtID(ctxTx, resolver.Target.Value, false)
	if err != nil {
		return ctx, metadb.FolderCoords{}, nil, xerrors.Errorf("folder coords for folder ext id %q not retrieved: %w", resolver.Target.Value, err)
	}

	return ctxTx, fcoords, []as.Resource{as.ResourceFolder(fcoords.FolderExtID)}, nil
}

func (s *Sessions) prepareAuthByCloudExtID(ctx context.Context, ns sqlutil.NodeStateCriteria, resolver sessions.SessionResolver) (context.Context, metadb.FolderCoords, []as.Resource, error) {
	ctxTx, err := s.metaDB.Begin(ctx, ns)
	if err != nil {
		return ctx, metadb.FolderCoords{}, nil, err
	}
	defer func() {
		s.cleanupTxContext(ctxTx, err)
	}()
	cloud, err := s.cloudByExtID(ctxTx, resolver.Target.Value, false)
	if err != nil {
		return ctx, metadb.FolderCoords{}, nil, err
	}
	result := metadb.FolderCoords{
		CloudExtID: cloud.CloudExtID,
		CloudID:    cloud.CloudID,
	}
	return ctxTx, result, []as.Resource{as.ResourceCloud(resolver.Target.Value)}, nil
}

func (s *Sessions) prepareAuthByOperationID(ctx context.Context, resolver sessions.SessionResolver) (context.Context, metadb.FolderCoords, []as.Resource, error) {
	// Always request Primary - primary/standby races otherwise
	ctxTx, err := s.metaDB.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return ctx, metadb.FolderCoords{}, nil, err
	}
	defer func() {
		s.cleanupTxContext(ctxTx, err)
	}()

	fcoords, err := s.metaDB.FolderCoordsByOperationID(ctxTx, resolver.Target.Value)
	if err != nil {
		if xerrors.Is(err, sqlerrors.ErrNotFound) {
			return ctx, metadb.FolderCoords{}, nil, resolver.ErrorResolver(semerr.NotFoundf("operation id %q not found", resolver.Target.Value))
		}

		return ctx, metadb.FolderCoords{}, nil, xerrors.Errorf("folder coords for cluster id %q not retrieved: %w", resolver.Target.Value, err)
	}

	return ctxTx, fcoords, []as.Resource{as.ResourceFolder(fcoords.FolderExtID)}, nil
}

var errNestedSession = xerrors.NewSentinel("attempted to nest session")

func (s *Sessions) Begin(ctx context.Context, resolver sessions.SessionResolver, opts ...sessions.SessionOption) (context.Context, sessions.Session, error) {
	if sessionMarkerFrom(ctx) {
		return ctx, sessions.Session{}, errNestedSession.WithStackTrace()
	}

	ns := sqlutil.Primary
	if resolver.Target.Permissions.IsGet() {
		ns = sqlutil.Alive
	}

	// Apply options to NodeStateCriteria
	for _, opt := range opts {
		opt(&ns)
	}

	if err := resolver.Validate(); err != nil {
		return ctx, sessions.Session{}, err
	}

	// Start tx to metadb, preload cloud and folder ids if needed, set target resources
	ctx, fcoords, resources, err := s.prepareAuth(ctx, ns, resolver)
	if err != nil {
		return ctx, sessions.Session{}, err
	}
	defer func() {
		s.cleanupTxContext(ctx, err)
	}()

	authRes, err := s.auth.Authenticate(ctx, resolver.Target.Permissions, resources...)
	if err != nil {
		return ctx, sessions.Session{}, err
	}
	ctx = withUserFields(ctx, authRes)

	fcoords, err = s.createFolder(ctx, fcoords, resolver)
	if err != nil {
		return ctx, sessions.Session{}, err
	}

	var cloud metadb.Cloud
	if resolver.Target.Permissions.IsGet() {
		// TODO: this must be loaded somewhere above during authentication
		if cloud, err = s.cloudByExtID(ctx, fcoords.CloudExtID, ns == sqlutil.Primary); err != nil {
			return ctx, sessions.Session{}, xerrors.Errorf("cloud %q not loaded: %w", fcoords.CloudExtID, err)
		}
	} else {
		if cloud, err = s.metaDB.LockCloud(ctx, fcoords.CloudID); err != nil {
			return ctx, sessions.Session{}, xerrors.Errorf("cloud %d %v not locked: %w", fcoords.CloudID, fcoords, err)
		}
	}

	// TODO: this must be loaded somewhere above during authentication
	featureFlags, err := s.loadFeatureFlags(ctx, fcoords.CloudExtID)
	if err != nil {
		return ctx, sessions.Session{}, xerrors.Errorf("getting feature flags for cloud %q. %w", fcoords.CloudExtID, err)
	}

	session := sessions.NewSession(
		fcoords,
		authRes,
		featureFlags,
		quota.NewConsumption(quota.Resources(cloud.Quota), quota.Resources(cloud.Used)),
		s.auth,
		nil,
		nil,
	)

	if resolver.Target.Type == auth.TargetTypeClusterIDAndDestinationFolderID {
		_, dstFolderExtID, err := resolver.Target.GetClusterIDAndFolderExtID()
		if err != nil {
			return ctx, sessions.Session{}, err
		}

		dstFolderCoords, err := s.metaDB.FolderCoordsByFolderExtID(ctx, dstFolderExtID)
		if err != nil {
			if !xerrors.Is(err, sqlerrors.ErrNotFound) {
				return ctx, sessions.Session{}, err
			}

			err = session.Authorize(ctx, models.PermMDBAllCreate, as.ResourceFolder(dstFolderExtID))
			if err != nil {
				return ctx, sessions.Session{}, err
			}

			dstFolderCoords, err = s.folderCoordsByExtID(ctx, dstFolderExtID, true)
			if err != nil {
				return ctx, sessions.Session{}, err
			}
		}
		session.DstFolderCoords = &dstFolderCoords
		if dstFolderCoords.CloudID != session.FolderCoords.CloudID {
			dstCloud, err := s.metaDB.LockCloud(ctx, dstFolderCoords.CloudID)
			if err != nil {
				return ctx, sessions.Session{}, xerrors.Errorf("locking destination cloud %q. %w", dstFolderCoords.CloudExtID, err)
			}
			session.DstQuota = quota.NewConsumption(quota.Resources(dstCloud.Quota), quota.Resources(dstCloud.Used))
		}
	}

	return withSessionMarker(ctx), session, nil
}

func withUserFields(ctx context.Context, authRes as.Subject) context.Context {
	accType := string(authRes.AccountType())
	tags.UserType.SetContext(ctx, accType)
	request.SetRequestUserType(ctx, accType)

	userFields := []log.Field{log.String("user_type", accType)}

	userID, err := authRes.ID()
	if err == nil {
		tags.UserID.SetContext(ctx, userID)
		userFields = append(userFields, log.String("user_id", userID))
		request.SetRequestUserID(ctx, userID)
	}

	ctx = ctxlog.WithFields(ctx, userFields...)

	return ctx
}

func (s *Sessions) BeginWithIdempotence(
	ctx context.Context,
	resolver sessions.SessionResolver,
	opts ...sessions.SessionOption,
) (context.Context, sessions.Session, sessions.OriginalRequest, error) {
	ctx, session, err := s.Begin(ctx, resolver, opts...)
	if err != nil {
		return ctx, sessions.Session{}, sessions.OriginalRequest{}, err
	}

	original, err := s.checkIdempotence(ctx, session)
	if err != nil {
		return ctx, sessions.Session{}, sessions.OriginalRequest{}, err
	}

	return ctx, session, original, nil
}

func (s *Sessions) checkIdempotence(ctx context.Context, session sessions.Session) (sessions.OriginalRequest, error) {
	idemp, ok := idempotence.IncomingFromContext(ctx)
	if !ok {
		return sessions.OriginalRequest{}, nil
	}

	opID, hash, err := s.metaDB.OperationIDByIdempotenceID(ctx, idemp.ID, session.Subject.MustID(), session.FolderCoords.FolderID)
	if err != nil {
		if xerrors.Is(err, sqlerrors.ErrNotFound) {
			return sessions.OriginalRequest{}, nil
		}

		return sessions.OriginalRequest{}, xerrors.Errorf("failed to retrieve operation ID by idempotence ID: %w", err)
	}

	if !bytes.Equal(idemp.Hash, hash) {
		return sessions.OriginalRequest{}, semerr.InvalidInputf("operation requests with idempotence option set must have identical request bodies")
	}

	op, err := s.metaDB.OperationByID(ctx, opID)
	if err != nil {
		return sessions.OriginalRequest{}, xerrors.Errorf("failed to retrieve existing operation for idempotent request: %w", err)
	}

	return sessions.OriginalRequest{Op: op, Exists: true}, nil
}

func (s *Sessions) Commit(ctx context.Context) error {
	return s.metaDB.Commit(ctx)
}

func (s *Sessions) Rollback(ctx context.Context) {
	_ = s.metaDB.Rollback(ctx)
}
