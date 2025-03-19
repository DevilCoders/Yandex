package provider

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	resmanMock "a.yandex-team.ru/cloud/mdb/internal/compute/resmanager/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	authMock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/auth/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	dbMock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/quota"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestBeginValidationNestedSessions(t *testing.T) {
	t.Run("NestedSessions", func(t *testing.T) {
		folderCoords := metadb.FolderCoords{
			CloudID:     1,
			CloudExtID:  "cloud1",
			FolderID:    10,
			FolderExtID: "folder1",
		}
		cloud := metadb.Cloud{
			CloudID:    1,
			CloudExtID: "cloud1",
		}

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderCoords.FolderExtID).Return(folderCoords, nil)
		db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
		db.EXPECT().CloudByCloudExtID(gomock.Any(), cloud.CloudExtID).Return(cloud, nil)
		rmClient := resmanMock.NewMockClient(ctrl)
		rmClient.EXPECT().PermissionStages(gomock.Any(), gomock.Any()).Return([]string{}, nil)
		auth := authMock.NewMockAuthenticator(ctrl)
		auth.EXPECT().Authenticate(gomock.Any(), models.Permission(models.PermMDBAllCreate), gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{}, nil)
		db.EXPECT().LockCloud(gomock.Any(), cloud.CloudID)
		sess := NewSessions(db, auth, rmClient, quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByFolder(folderCoords.FolderExtID, models.PermMDBAllCreate)
		ctx, _, err := sess.Begin(context.Background(), resolver)
		require.NoError(t, err)
		_, _, err = sess.Begin(ctx, resolver)
		require.True(t, xerrors.Is(err, errNestedSession))
	})

	t.Run("Resolver", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		rmClient := resmanMock.NewMockClient(ctrl)
		auth := authMock.NewMockAuthenticator(ctrl)
		sess := NewSessions(db, auth, rmClient, quota.Resources{}, environment.ResourceModelYandex)

		// We test for existence of validation, not for every possible use-case
		// Validations must be tested near code that does actual validation
		resolver := sessions.ResolveByCluster("", models.PermMDBAllRead)
		_, _, err := sess.Begin(context.Background(), resolver)
		require.Error(t, err)
	})
}

type testKey string

const (
	txKey      = testKey("tx")
	stanbyKey  = testKey("standby")
	primaryKey = testKey("primary")

	txVal      = "tx"
	stanbyVal  = "standby"
	primaryVal = "primary"
)

func withTx(ctx context.Context) context.Context {
	return context.WithValue(ctx, txKey, txVal)
}

func hasTx(ctx context.Context) bool {
	return ctx.Value(txKey) == txVal
}

func withStandby(ctx context.Context) context.Context {
	return context.WithValue(ctx, stanbyKey, stanbyVal)
}

func withPrimary(ctx context.Context) context.Context {
	return context.WithValue(ctx, primaryKey, primaryVal)
}

func TestPrepareAuthByClusterID(t *testing.T) {
	t.Run("LoadAndUsePrimary", func(t *testing.T) {
		cid := "cid1"
		fcoords := metadb.FolderCoords{
			CloudID:     2,
			CloudExtID:  "cloud2",
			FolderID:    20,
			FolderExtID: "folder2",
		}
		ns := sqlutil.Primary
		perms := models.PermMDBAllModify
		rev := int64(1)

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxWithTx := withTx(ctx)
		beginCall := db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctxWithTx, nil)
		db.EXPECT().FolderCoordsByClusterID(ctxWithTx, cid, models.VisibilityVisible).Return(fcoords, rev, clusters.TypeKafka, nil).After(beginCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByCluster(cid, perms)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByClusterID(ctx, ns, resolver)
		require.NoError(t, err)
		assert.Equal(t, ctxWithTx, ctxRes)
		assert.Equal(t, fcoords, fcoordsRes)
		assert.Equal(t, []as.Resource{{Type: "managed-kafka.cluster", ID: "cid1"}, as.ResourceFolder(fcoords.FolderExtID)}, resourcesRes)
	})

	t.Run("LoadStandbyAndPrimaryStaleStandby", func(t *testing.T) {
		cid := "cid1"
		fcoordsPrimary := metadb.FolderCoords{
			CloudID:     3,
			CloudExtID:  "cloud3",
			FolderID:    30,
			FolderExtID: "folder3",
		}
		fcoordsStandby := metadb.FolderCoords{
			CloudID:     2,
			CloudExtID:  "cloud2",
			FolderID:    20,
			FolderExtID: "folder2",
		}
		perms := models.PermMDBAllRead
		revPrimary := int64(2)
		revStandby := int64(1)

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxStandby := withStandby(ctx)
		ctxPrimary := withPrimary(ctx)
		beginStandbyCall := db.EXPECT().Begin(ctx, sqlutil.Standby).Return(ctxStandby, nil)
		fcByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxStandby, cid, models.VisibilityVisible).Return(fcoordsStandby, revStandby, clusters.TypeKafka, nil).After(beginStandbyCall)
		beginPrimaryCall := db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctxPrimary, nil).After(fcByClusterIDCall)
		db.EXPECT().FolderCoordsByClusterID(ctxPrimary, cid, models.VisibilityVisible).Return(fcoordsPrimary, revPrimary, clusters.TypeKafka, nil).After(beginPrimaryCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByCluster(cid, perms)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByClusterID(ctx, sqlutil.Alive, resolver)
		require.NoError(t, err)
		assert.Equal(t, ctxPrimary, ctxRes)
		assert.Equal(t, fcoordsPrimary, fcoordsRes)
		assert.Equal(t, []as.Resource{{Type: "managed-kafka.cluster", ID: "cid1"}, as.ResourceFolder(fcoordsPrimary.FolderExtID)}, resourcesRes)
	})

	t.Run("LoadStandbyAndPrimaryUpToDateStandby", func(t *testing.T) {
		cid := "cid1"
		fcoordsPrimary := metadb.FolderCoords{
			CloudID:     3,
			CloudExtID:  "cloud3",
			FolderID:    30,
			FolderExtID: "folder3",
		}
		fcoordsStandby := metadb.FolderCoords{
			CloudID:     2,
			CloudExtID:  "cloud2",
			FolderID:    20,
			FolderExtID: "folder2",
		}
		perms := models.PermMDBAllRead
		revPrimary := int64(2)
		revStandby := revPrimary

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxStandby := withStandby(ctx)
		ctxPrimary := withPrimary(ctx)
		beginStandbyCall := db.EXPECT().Begin(ctx, sqlutil.Standby).Return(ctxStandby, nil)
		fcByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxStandby, cid, models.VisibilityVisible).Return(fcoordsStandby, revStandby, clusters.TypeKafka, nil).After(beginStandbyCall)
		beginPrimaryCall := db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctxPrimary, nil).After(fcByClusterIDCall)
		db.EXPECT().FolderCoordsByClusterID(ctxPrimary, cid, models.VisibilityVisible).Return(fcoordsPrimary, revPrimary, clusters.TypeKafka, nil).After(beginPrimaryCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByCluster(cid, perms)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByClusterID(ctx, sqlutil.Alive, resolver)
		require.NoError(t, err)
		assert.Equal(t, ctxStandby, ctxRes)
		assert.Equal(t, fcoordsStandby, fcoordsRes)
		assert.Equal(t, []as.Resource{{Type: "managed-kafka.cluster", ID: "cid1"}, as.ResourceFolder(fcoordsStandby.FolderExtID)}, resourcesRes)
	})

	t.Run("LoadPrimaryBeginError", func(t *testing.T) {
		cid := "cid1"
		ns := sqlutil.Primary
		perms := models.PermMDBAllModify

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		expectedErr := xerrors.New("BeginError")
		db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctx, expectedErr)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByCluster(cid, perms)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByClusterID(ctx, ns, resolver)
		require.Error(t, err)
		assert.True(t, xerrors.Is(err, expectedErr))
		assert.Equal(t, ctx, ctxRes)
		assert.Equal(t, metadb.FolderCoords{}, fcoordsRes)
		assert.Len(t, resourcesRes, 0)
	})

	t.Run("LoadPrimaryError", func(t *testing.T) {
		cid := "cid1"
		ns := sqlutil.Primary
		perms := models.PermMDBAllModify

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxWithTx := withTx(ctx)
		expectedErr := xerrors.New("FolderCoordsByClusterIDError")
		beginCall := db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctxWithTx, nil)
		fcByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxWithTx, cid, models.VisibilityVisible).Return(metadb.FolderCoords{}, int64(0), clusters.TypeUnknown, expectedErr).After(beginCall)
		db.EXPECT().Rollback(ctxWithTx).After(fcByClusterIDCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByCluster(cid, perms)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByClusterID(ctx, ns, resolver)
		require.Error(t, err)
		assert.True(t, xerrors.Is(err, expectedErr))
		assert.Equal(t, ctx, ctxRes)
		assert.Equal(t, metadb.FolderCoords{}, fcoordsRes)
		assert.Len(t, resourcesRes, 0)
	})

	t.Run("LoadPrimaryNotFound", func(t *testing.T) {
		cid := "cid1"
		ns := sqlutil.Primary
		perms := models.PermMDBAllModify

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxWithTx := withTx(ctx)
		beginCall := db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctxWithTx, nil)
		fcByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxWithTx, cid, models.VisibilityVisible).Return(metadb.FolderCoords{}, int64(0), clusters.TypeUnknown, sqlerrors.ErrNotFound).After(beginCall)
		db.EXPECT().Rollback(ctxWithTx).After(fcByClusterIDCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByCluster(cid, perms)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByClusterID(ctx, ns, resolver)
		require.Error(t, err)
		assert.True(t, semerr.IsNotFound(err))
		assert.Equal(t, ctx, ctxRes)
		assert.Equal(t, metadb.FolderCoords{}, fcoordsRes)
		assert.Len(t, resourcesRes, 0)
	})

	t.Run("LoadStandbyAndPrimaryErrorOnStandbyBegin", func(t *testing.T) {
		cid := "cid1"
		fcoordsPrimary := metadb.FolderCoords{
			CloudID:     3,
			CloudExtID:  "cloud3",
			FolderID:    30,
			FolderExtID: "folder3",
		}
		perms := models.PermMDBAllRead
		revPrimary := int64(2)

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxPrimary := withPrimary(ctx)
		expectedErr := xerrors.New("BeginError")
		beginStandbyCall := db.EXPECT().Begin(ctx, sqlutil.Standby).Return(ctx, expectedErr)
		beginPrimaryCall := db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctxPrimary, nil).After(beginStandbyCall)
		db.EXPECT().FolderCoordsByClusterID(ctxPrimary, cid, models.VisibilityVisible).Return(fcoordsPrimary, revPrimary, clusters.TypeKafka, nil).After(beginPrimaryCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByCluster(cid, perms)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByClusterID(ctx, sqlutil.Alive, resolver)
		require.NoError(t, err)
		assert.Equal(t, ctxPrimary, ctxRes)
		assert.Equal(t, fcoordsPrimary, fcoordsRes)
		assert.Equal(t, []as.Resource{{Type: "managed-kafka.cluster", ID: "cid1"}, as.ResourceFolder(fcoordsPrimary.FolderExtID)}, resourcesRes)
	})

	t.Run("LoadStandbyAndPrimaryErrorOnStandbyLoad", func(t *testing.T) {
		cid := "cid1"
		fcoordsPrimary := metadb.FolderCoords{
			CloudID:     3,
			CloudExtID:  "cloud3",
			FolderID:    30,
			FolderExtID: "folder3",
		}
		perms := models.PermMDBAllRead
		revPrimary := int64(2)

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxStandby := withStandby(ctx)
		ctxPrimary := withPrimary(ctx)
		expectedErr := xerrors.New("FolderCoordsByClusterIDError")
		beginStandbyCall := db.EXPECT().Begin(ctx, sqlutil.Standby).Return(ctxStandby, nil)
		fcStandbyByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxStandby, cid, models.VisibilityVisible).Return(metadb.FolderCoords{}, int64(0), clusters.TypeKafka, expectedErr).After(beginStandbyCall)
		beginPrimaryCall := db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctxPrimary, nil).After(fcStandbyByClusterIDCall)
		fcPrimaryByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxPrimary, cid, models.VisibilityVisible).Return(fcoordsPrimary, revPrimary, clusters.TypeKafka, nil).After(beginPrimaryCall)
		db.EXPECT().Rollback(ctxStandby).After(fcPrimaryByClusterIDCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByCluster(cid, perms)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByClusterID(ctx, sqlutil.Alive, resolver)
		require.NoError(t, err)
		assert.Equal(t, ctxPrimary, ctxRes)
		assert.Equal(t, fcoordsPrimary, fcoordsRes)
		assert.Equal(t, []as.Resource{{Type: "managed-kafka.cluster", ID: "cid1"}, as.ResourceFolder(fcoordsPrimary.FolderExtID)}, resourcesRes)
	})

	t.Run("LoadStandbyAndPrimaryNotFoundOnStandbyLoad", func(t *testing.T) {
		cid := "cid1"
		fcoordsPrimary := metadb.FolderCoords{
			CloudID:     3,
			CloudExtID:  "cloud3",
			FolderID:    30,
			FolderExtID: "folder3",
		}
		perms := models.PermMDBAllRead
		revPrimary := int64(2)

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxStandby := withStandby(ctx)
		ctxPrimary := withPrimary(ctx)
		beginStandbyCall := db.EXPECT().Begin(ctx, sqlutil.Standby).Return(ctxStandby, nil)
		fcStandbyByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxStandby, cid, models.VisibilityVisible).Return(metadb.FolderCoords{}, int64(0), clusters.TypeKafka, sqlerrors.ErrNotFound).After(beginStandbyCall)
		beginPrimaryCall := db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctxPrimary, nil).After(fcStandbyByClusterIDCall)
		fcPrimaryByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxPrimary, cid, models.VisibilityVisible).Return(fcoordsPrimary, revPrimary, clusters.TypeKafka, nil).After(beginPrimaryCall)
		db.EXPECT().Rollback(ctxStandby).After(fcPrimaryByClusterIDCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByCluster(cid, perms)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByClusterID(ctx, sqlutil.Alive, resolver)
		require.NoError(t, err)
		assert.Equal(t, ctxPrimary, ctxRes)
		assert.Equal(t, fcoordsPrimary, fcoordsRes)
		assert.Equal(t, []as.Resource{{Type: "managed-kafka.cluster", ID: "cid1"}, as.ResourceFolder(fcoordsPrimary.FolderExtID)}, resourcesRes)
	})

	t.Run("LoadStandbyAndPrimaryErrorOnPrimaryBegin", func(t *testing.T) {
		cid := "cid1"
		fcoordsStandby := metadb.FolderCoords{
			CloudID:     3,
			CloudExtID:  "cloud3",
			FolderID:    30,
			FolderExtID: "folder3",
		}
		perms := models.PermMDBAllRead
		revStandby := int64(2)

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxStandby := withPrimary(ctx)
		expectedErr := xerrors.New("BeginError")
		beginPrimaryCall := db.EXPECT().Begin(ctx, sqlutil.Standby).Return(ctxStandby, nil)
		fcByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxStandby, cid, models.VisibilityVisible).Return(fcoordsStandby, revStandby, clusters.TypeKafka, nil).After(beginPrimaryCall)
		db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctx, expectedErr).After(fcByClusterIDCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByCluster(cid, perms)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByClusterID(ctx, sqlutil.Alive, resolver)
		require.NoError(t, err)
		assert.Equal(t, ctxStandby, ctxRes)
		assert.Equal(t, fcoordsStandby, fcoordsRes)
		assert.Equal(t, []as.Resource{{Type: "managed-kafka.cluster", ID: "cid1"}, as.ResourceFolder(fcoordsStandby.FolderExtID)}, resourcesRes)
	})

	t.Run("LoadStandbyAndPrimaryErrorOnPrimaryLoad", func(t *testing.T) {
		cid := "cid1"
		fcoordsStandby := metadb.FolderCoords{
			CloudID:     3,
			CloudExtID:  "cloud3",
			FolderID:    30,
			FolderExtID: "folder3",
		}
		perms := models.PermMDBAllRead
		revStandby := int64(2)

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxStandby := withStandby(ctx)
		ctxPrimary := withPrimary(ctx)
		expectedErr := xerrors.New("FolderCoordsByClusterIDError")
		beginStandbyCall := db.EXPECT().Begin(ctx, sqlutil.Standby).Return(ctxStandby, nil)
		fcStandbyByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxStandby, cid, models.VisibilityVisible).Return(fcoordsStandby, revStandby, clusters.TypeKafka, nil).After(beginStandbyCall)
		beginPrimaryCall := db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctxPrimary, nil).After(fcStandbyByClusterIDCall)
		fcPrimaryByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxPrimary, cid, models.VisibilityVisible).Return(metadb.FolderCoords{}, int64(0), clusters.TypeKafka, expectedErr).After(beginPrimaryCall)
		db.EXPECT().Rollback(ctxPrimary).After(fcPrimaryByClusterIDCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByCluster(cid, perms)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByClusterID(ctx, sqlutil.Alive, resolver)
		require.NoError(t, err)
		assert.Equal(t, ctxStandby, ctxRes)
		assert.Equal(t, fcoordsStandby, fcoordsRes)
		assert.Equal(t, []as.Resource{{Type: "managed-kafka.cluster", ID: "cid1"}, as.ResourceFolder(fcoordsStandby.FolderExtID)}, resourcesRes)
	})

	t.Run("LoadStandbyAndPrimaryNotFoundOnPrimaryLoad", func(t *testing.T) {
		cid := "cid1"
		fcoordsStandby := metadb.FolderCoords{
			CloudID:     3,
			CloudExtID:  "cloud3",
			FolderID:    30,
			FolderExtID: "folder3",
		}
		perms := models.PermMDBAllRead
		revStandby := int64(2)

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxStandby := withStandby(ctx)
		ctxPrimary := withPrimary(ctx)
		beginStandbyCall := db.EXPECT().Begin(ctx, sqlutil.Standby).Return(ctxStandby, nil)
		fcStandbyByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxStandby, cid, models.VisibilityVisible).Return(fcoordsStandby, revStandby, clusters.TypeUnknown, nil).After(beginStandbyCall)
		beginPrimaryCall := db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctxPrimary, nil).After(fcStandbyByClusterIDCall)
		fcPrimaryByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxPrimary, cid, models.VisibilityVisible).Return(metadb.FolderCoords{}, int64(0), clusters.TypeUnknown, sqlerrors.ErrNotFound).After(beginPrimaryCall)
		rollbackStandbyCall := db.EXPECT().Rollback(ctxPrimary).After(fcPrimaryByClusterIDCall)
		db.EXPECT().Rollback(ctxStandby).After(rollbackStandbyCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByCluster(cid, perms)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByClusterID(ctx, sqlutil.Alive, resolver)
		require.Error(t, err)
		assert.True(t, semerr.IsNotFound(err))
		assert.Equal(t, ctx, ctxRes)
		assert.Equal(t, metadb.FolderCoords{}, fcoordsRes)
		assert.Len(t, resourcesRes, 0)
	})

	t.Run("LoadStandbyAndPrimaryErrorOnStandbyAndPrimaryLoad", func(t *testing.T) {
		cid := "cid1"
		perms := models.PermMDBAllRead

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxStandby := withStandby(ctx)
		ctxPrimary := withPrimary(ctx)
		standbyErr := xerrors.New("standbyError")
		primaryErr := xerrors.New("primaryError")
		beginStandbyCall := db.EXPECT().Begin(ctx, sqlutil.Standby).Return(ctxStandby, nil)
		fcStandbyByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxStandby, cid, models.VisibilityVisible).Return(metadb.FolderCoords{}, int64(0), clusters.TypeUnknown, standbyErr).After(beginStandbyCall)
		beginPrimaryCall := db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctxPrimary, nil).After(fcStandbyByClusterIDCall)
		fcPrimaryByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxPrimary, cid, models.VisibilityVisible).Return(metadb.FolderCoords{}, int64(0), clusters.TypeUnknown, primaryErr).After(beginPrimaryCall)
		rollbackStandbyCall := db.EXPECT().Rollback(ctxPrimary).After(fcPrimaryByClusterIDCall)
		db.EXPECT().Rollback(ctxStandby).After(rollbackStandbyCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByCluster(cid, perms)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByClusterID(ctx, sqlutil.Alive, resolver)
		require.Error(t, err)
		assert.True(t, xerrors.Is(err, primaryErr))
		assert.Equal(t, ctx, ctxRes)
		assert.Equal(t, metadb.FolderCoords{}, fcoordsRes)
		assert.Len(t, resourcesRes, 0)
	})

	t.Run("LoadStandbyAndPrimaryNotFoundOnStandbyLoadErrorOnPrimaryLoad", func(t *testing.T) {
		cid := "cid1"
		perms := models.PermMDBAllRead

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxStandby := withStandby(ctx)
		ctxPrimary := withPrimary(ctx)
		primaryErr := xerrors.New("primaryError")
		beginStandbyCall := db.EXPECT().Begin(ctx, sqlutil.Standby).Return(ctxStandby, nil)
		fcStandbyByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxStandby, cid, models.VisibilityVisible).Return(metadb.FolderCoords{}, int64(0), clusters.TypeUnknown, sqlerrors.ErrNotFound).After(beginStandbyCall)
		beginPrimaryCall := db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctxPrimary, nil).After(fcStandbyByClusterIDCall)
		fcPrimaryByClusterIDCall := db.EXPECT().FolderCoordsByClusterID(ctxPrimary, cid, models.VisibilityVisible).Return(metadb.FolderCoords{}, int64(0), clusters.TypeUnknown, primaryErr).After(beginPrimaryCall)
		rollbackStandbyCall := db.EXPECT().Rollback(ctxPrimary).After(fcPrimaryByClusterIDCall)
		db.EXPECT().Rollback(ctxStandby).After(rollbackStandbyCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByCluster(cid, perms)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByClusterID(ctx, sqlutil.Alive, resolver)
		require.Error(t, err)
		assert.True(t, semerr.IsNotFound(err))
		assert.Equal(t, ctx, ctxRes)
		assert.Equal(t, metadb.FolderCoords{}, fcoordsRes)
		assert.Len(t, resourcesRes, 0)
	})
}

func TestPrepareAuthByFolderExtID(t *testing.T) {
	t.Run("Ok", func(t *testing.T) {
		folderExtID := "folderExtID1"
		fcoords := metadb.FolderCoords{
			CloudID:     2,
			CloudExtID:  "cloud2",
			FolderID:    20,
			FolderExtID: "folder2",
		}

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxWithTx := withTx(ctx)
		beginCall := db.EXPECT().Begin(ctx, sqlutil.Alive).Return(ctxWithTx, nil)
		db.EXPECT().FolderCoordsByFolderExtID(ctxWithTx, folderExtID).Return(fcoords, nil).After(beginCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByFolder(folderExtID, models.PermMDBAllRead)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByFolderExtID(ctx, sqlutil.Alive, resolver)
		require.NoError(t, err)
		assert.Equal(t, ctxWithTx, ctxRes)
		assert.Equal(t, fcoords, fcoordsRes)
		assert.Equal(t, []as.Resource{as.ResourceFolder(fcoords.FolderExtID)}, resourcesRes)
	})

	t.Run("BeginError", func(t *testing.T) {
		folderExtID := "folderExtID1"

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		expectedErr := xerrors.New("BeginError")
		db.EXPECT().Begin(ctx, sqlutil.Alive).Return(ctx, expectedErr)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByFolder(folderExtID, models.PermMDBAllRead)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByFolderExtID(ctx, sqlutil.Alive, resolver)
		require.Error(t, err)
		assert.True(t, xerrors.Is(err, expectedErr))
		assert.Equal(t, ctx, ctxRes)
		assert.Equal(t, metadb.FolderCoords{}, fcoordsRes)
		assert.Len(t, resourcesRes, 0)
	})

	t.Run("Error", func(t *testing.T) {
		folderExtID := "folderExtID1"

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxWithTx := withTx(ctx)
		expectedErr := xerrors.New("FolderCoordsByOperationIDError")
		beginCall := db.EXPECT().Begin(ctx, sqlutil.Alive).Return(ctxWithTx, nil)
		fcByFolderExtIDCall := db.EXPECT().FolderCoordsByFolderExtID(ctxWithTx, folderExtID).Return(metadb.FolderCoords{}, expectedErr).After(beginCall)
		db.EXPECT().Rollback(ctxWithTx).After(fcByFolderExtIDCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByFolder(folderExtID, models.PermMDBAllRead)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByFolderExtID(ctx, sqlutil.Alive, resolver)
		require.Error(t, err)
		assert.True(t, xerrors.Is(err, expectedErr))
		assert.Equal(t, ctx, ctxRes)
		assert.Equal(t, metadb.FolderCoords{}, fcoordsRes)
		assert.Len(t, resourcesRes, 0)
	})
}

func TestPrepareAuthByCloudExtID(t *testing.T) {
	t.Run("Ok", func(t *testing.T) {
		cloudExtID := "cloudExtID1"

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxWithTx := withTx(ctx)
		db.EXPECT().Begin(ctx, sqlutil.Alive).Return(ctxWithTx, nil)
		db.EXPECT().CloudByCloudExtID(ctxWithTx, cloudExtID).Return(metadb.Cloud{
			CloudID:    1,
			CloudExtID: cloudExtID,
		}, nil)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByCloud(cloudExtID, models.PermMDBAllRead)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByCloudExtID(ctx, sqlutil.Alive, resolver)
		require.NoError(t, err)
		assert.Equal(t, ctxWithTx, ctxRes)
		assert.Equal(t, metadb.FolderCoords{CloudExtID: cloudExtID, CloudID: 1}, fcoordsRes)
		assert.Equal(t, []as.Resource{as.ResourceCloud(cloudExtID)}, resourcesRes)
	})

	t.Run("BeginError", func(t *testing.T) {
		cloudExtID := "cloudExtID1"

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		expectedErr := xerrors.New("BeginError")
		db.EXPECT().Begin(ctx, sqlutil.Alive).Return(ctx, expectedErr)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByCloud(cloudExtID, models.PermMDBAllRead)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByCloudExtID(ctx, sqlutil.Alive, resolver)
		require.Error(t, err)
		assert.True(t, xerrors.Is(err, expectedErr))
		assert.Equal(t, ctx, ctxRes)
		assert.Equal(t, metadb.FolderCoords{}, fcoordsRes)
		assert.Len(t, resourcesRes, 0)
	})
}

func TestPrepareAuthByOperationID(t *testing.T) {
	t.Run("Ok", func(t *testing.T) {
		opID := "operation1"
		fcoords := metadb.FolderCoords{
			CloudID:     2,
			CloudExtID:  "cloud2",
			FolderID:    20,
			FolderExtID: "folder2",
		}

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxWithTx := withTx(ctx)
		beginCall := db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctxWithTx, nil)
		db.EXPECT().FolderCoordsByOperationID(ctxWithTx, opID).Return(fcoords, nil).After(beginCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByOperation(opID)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByOperationID(ctx, resolver)
		require.NoError(t, err)
		assert.Equal(t, ctxWithTx, ctxRes)
		assert.Equal(t, fcoords, fcoordsRes)
		assert.Equal(t, []as.Resource{as.ResourceFolder(fcoords.FolderExtID)}, resourcesRes)
	})

	t.Run("BeginError", func(t *testing.T) {
		opID := "operation1"

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		expectedErr := xerrors.New("BeginError")
		db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctx, expectedErr)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByOperation(opID)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByOperationID(ctx, resolver)
		require.Error(t, err)
		assert.True(t, xerrors.Is(err, expectedErr))
		assert.Equal(t, ctx, ctxRes)
		assert.Equal(t, metadb.FolderCoords{}, fcoordsRes)
		assert.Len(t, resourcesRes, 0)
	})

	t.Run("Error", func(t *testing.T) {
		opID := "operation1"

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxWithTx := withTx(ctx)
		expectedErr := xerrors.New("FolderCoordsByOperationIDError")
		beginCall := db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctxWithTx, nil)
		fcByOpIDCall := db.EXPECT().FolderCoordsByOperationID(ctxWithTx, opID).Return(metadb.FolderCoords{}, expectedErr).After(beginCall)
		db.EXPECT().Rollback(ctxWithTx).After(fcByOpIDCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByOperation(opID)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByOperationID(ctx, resolver)
		require.Error(t, err)
		assert.True(t, xerrors.Is(err, expectedErr))
		assert.Equal(t, ctx, ctxRes)
		assert.Equal(t, metadb.FolderCoords{}, fcoordsRes)
		assert.Len(t, resourcesRes, 0)
	})

	t.Run("NotFound", func(t *testing.T) {
		opID := "operation1"

		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		db := dbMock.NewMockBackend(ctrl)
		ctx := context.Background()
		ctxWithTx := withTx(ctx)
		beginCall := db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctxWithTx, nil)
		fcByOpIDCall := db.EXPECT().FolderCoordsByOperationID(ctxWithTx, opID).Return(metadb.FolderCoords{}, sqlerrors.ErrNotFound).After(beginCall)
		db.EXPECT().Rollback(ctxWithTx).After(fcByOpIDCall)

		sess := NewSessions(db, authMock.NewMockAuthenticator(ctrl), resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
		resolver := sessions.ResolveByOperation(opID)
		ctxRes, fcoordsRes, resourcesRes, err := sess.prepareAuthByOperationID(ctx, resolver)
		require.Error(t, err)
		require.True(t, semerr.IsNotFound(err))
		assert.Equal(t, ctx, ctxRes)
		assert.Equal(t, metadb.FolderCoords{}, fcoordsRes)
		assert.Len(t, resourcesRes, 0)
	})
}

func TestCloudByExtIDExists(t *testing.T) {
	cloud := metadb.Cloud{
		CloudID:    1,
		CloudExtID: "cloud1",
	}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), cloud.CloudExtID).Return(cloud, nil)
	auth := authMock.NewMockAuthenticator(ctrl)
	sess := NewSessions(db, auth, resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
	got, err := sess.cloudByExtID(context.Background(), cloud.CloudExtID, false)
	require.NoError(t, err)
	require.Equal(t, cloud, got)
}

func TestCloudByExtIDNotFoundPassed(t *testing.T) {
	cloudNotFound := metadb.Cloud{
		CloudID:    notFoundID,
		CloudExtID: "cloud_not_found",
	}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), cloudNotFound.CloudExtID).Return(metadb.Cloud{}, sqlerrors.ErrNotFound)
	auth := authMock.NewMockAuthenticator(ctrl)
	sess := NewSessions(db, auth, resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
	got, err := sess.cloudByExtID(context.Background(), cloudNotFound.CloudExtID, false)
	require.NoError(t, err)
	require.Equal(t, cloudNotFound, got)
}

func TestCloudByExtIDErrorPassed(t *testing.T) {
	cloudErrorExtID := "cloud_error"

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), cloudErrorExtID).Return(metadb.Cloud{}, xerrors.New("Unhandled"))
	auth := authMock.NewMockAuthenticator(ctrl)
	sess := NewSessions(db, auth, resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
	_, err := sess.cloudByExtID(context.Background(), cloudErrorExtID, false)
	require.Nil(t, semerr.AsSemanticError(err))
}

func TestCloudByExtIDCreation(t *testing.T) {
	requestID := requestid.New()
	cloudCreated := metadb.Cloud{
		CloudID:    2,
		CloudExtID: "cloud2",
	}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), cloudCreated.CloudExtID).Return(metadb.Cloud{}, sqlerrors.ErrNotFound)
	db.EXPECT().CreateCloud(gomock.Any(), cloudCreated.CloudExtID, metadb.Resources{}, requestID).Return(cloudCreated, nil)
	auth := authMock.NewMockAuthenticator(ctrl)
	sess := NewSessions(db, auth, resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)

	ctx := requestid.WithRequestID(context.Background(), requestID)
	got, err := sess.cloudByExtID(ctx, cloudCreated.CloudExtID, true)
	require.NoError(t, err)
	require.Equal(t, cloudCreated, got)
}

func TestFolderCoordsByExtIDExists(t *testing.T) {
	folderCoords := metadb.FolderCoords{
		CloudID:     1,
		CloudExtID:  "cloud1",
		FolderID:    10,
		FolderExtID: "folder1",
	}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderCoords.FolderExtID).Return(folderCoords, nil)
	auth := authMock.NewMockAuthenticator(ctrl)
	sess := NewSessions(db, auth, resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
	got, err := sess.folderCoordsByExtID(context.Background(), folderCoords.FolderExtID, false)
	require.NoError(t, err)
	require.Equal(t, folderCoords, got)
}

func TestFolderCoordsByExtIDNotFound(t *testing.T) {
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), "folder_not_found").Return(metadb.FolderCoords{}, sqlerrors.ErrNotFound)

	resmanClient := resmanMock.NewMockClient(ctrl)
	resmanClient.EXPECT().ResolveFolders(gomock.Any(), []string{"folder_not_found"}).Return(nil, semerr.NotFound("not found"))
	auth := authMock.NewMockAuthenticator(ctrl)
	sess := NewSessions(db, auth, resmanClient, quota.Resources{}, environment.ResourceModelYandex)
	_, err := sess.folderCoordsByExtID(context.Background(), "folder_not_found", false)
	require.Error(t, err)
	require.True(t, semerr.IsNotFound(err))
}

func TestFolderCoordsByExtIDErrorPass(t *testing.T) {
	folderErrorExtID := "folder_error"

	ctrl := gomock.NewController(t)
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderErrorExtID).Return(metadb.FolderCoords{}, xerrors.New("Unhandled"))
	auth := authMock.NewMockAuthenticator(ctrl)
	sess := NewSessions(db, auth, resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)
	_, err := sess.folderCoordsByExtID(context.Background(), folderErrorExtID, false)
	require.Nil(t, semerr.AsSemanticError(err))
}

func TestFolderCoordsByExtIDHalfEmpty(t *testing.T) {
	folderCoordsNotFound := metadb.FolderCoords{
		CloudID:     notFoundID,
		CloudExtID:  "cloud2",
		FolderID:    notFoundID,
		FolderExtID: "folder2",
	}
	resolvedFolder := []resmanager.ResolvedFolder{{
		FolderExtID: folderCoordsNotFound.FolderExtID,
		CloudExtID:  folderCoordsNotFound.CloudExtID,
	}}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderCoordsNotFound.FolderExtID).Return(metadb.FolderCoords{}, sqlerrors.ErrNotFound)
	auth := authMock.NewMockAuthenticator(ctrl)
	resmanClient := resmanMock.NewMockClient(ctrl)
	resmanClient.EXPECT().ResolveFolders(gomock.Any(), []string{folderCoordsNotFound.FolderExtID}).Return(resolvedFolder, nil)
	sess := NewSessions(db, auth, resmanClient, quota.Resources{}, environment.ResourceModelYandex)

	got, err := sess.folderCoordsByExtID(context.Background(), folderCoordsNotFound.FolderExtID, false)
	require.NoError(t, err)
	require.Equal(t, folderCoordsNotFound, got)
}

func TestFolderCoordsByExtIDCreated(t *testing.T) {
	cloud := metadb.Cloud{
		CloudID:    2,
		CloudExtID: "cloud2",
	}
	folderCoordsCreated := metadb.FolderCoords{
		CloudID:     2,
		CloudExtID:  "cloud2",
		FolderID:    20,
		FolderExtID: "folder2",
	}
	resolvedFolder := []resmanager.ResolvedFolder{{
		CloudExtID:  folderCoordsCreated.CloudExtID,
		FolderExtID: folderCoordsCreated.FolderExtID,
	}}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderCoordsCreated.FolderExtID).Return(metadb.FolderCoords{}, sqlerrors.ErrNotFound)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), cloud.CloudExtID).Return(cloud, nil)
	db.EXPECT().CreateFolder(gomock.Any(), folderCoordsCreated.FolderExtID, folderCoordsCreated.CloudID).Return(folderCoordsCreated, nil)
	auth := authMock.NewMockAuthenticator(ctrl)
	resmanClient := resmanMock.NewMockClient(ctrl)
	resmanClient.EXPECT().ResolveFolders(gomock.Any(), []string{folderCoordsCreated.FolderExtID}).Return(resolvedFolder, nil)
	sess := NewSessions(db, auth, resmanClient, quota.Resources{}, environment.ResourceModelYandex)
	got, err := sess.folderCoordsByExtID(context.Background(), folderCoordsCreated.FolderExtID, true)
	require.NoError(t, err)
	require.Equal(t, folderCoordsCreated, got)
}

func TestBeginWithFolderExtIDExists(t *testing.T) {
	folderCoords := metadb.FolderCoords{
		CloudID:     1,
		CloudExtID:  "cloud1",
		FolderID:    10,
		FolderExtID: "folder1",
	}
	cloud := metadb.Cloud{
		CloudExtID: "cloud1",
		CloudID:    1,
	}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderCoords.FolderExtID).Return(folderCoords, nil)
	db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), folderCoords.CloudExtID).Return(cloud, nil)
	rmClient := resmanMock.NewMockClient(ctrl)
	rmClient.EXPECT().PermissionStages(gomock.Any(), folderCoords.CloudExtID).Return([]string{}, nil)
	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authenticate(gomock.Any(), models.Permission(models.PermMDBAllCreate), gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{}, nil)
	db.EXPECT().LockCloud(gomock.Any(), cloud.CloudID)
	sess := NewSessions(db, auth, rmClient, quota.Resources{}, environment.ResourceModelYandex)
	resolver := sessions.ResolveByFolder(folderCoords.FolderExtID, models.PermMDBAllCreate)
	_, s, err := sess.Begin(context.Background(), resolver)
	require.NoError(t, err)
	require.Equal(t, folderCoords, s.FolderCoords)
}

func TestBeginWithFolderExtIDExistsNoCloudLock(t *testing.T) {
	folderCoords := metadb.FolderCoords{
		CloudID:     1,
		CloudExtID:  "cloud1",
		FolderID:    10,
		FolderExtID: "folder1",
	}
	cloud := metadb.Cloud{
		CloudExtID: "cloud1",
		CloudID:    1,
	}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderCoords.FolderExtID).Return(folderCoords, nil)
	db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), folderCoords.CloudExtID).Return(cloud, nil).Times(2)
	rmClient := resmanMock.NewMockClient(ctrl)
	rmClient.EXPECT().PermissionStages(gomock.Any(), folderCoords.CloudExtID).Return([]string{}, nil)
	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authenticate(gomock.Any(), models.Permission(models.PermMDBAllRead), gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{}, nil)
	sess := NewSessions(db, auth, rmClient, quota.Resources{}, environment.ResourceModelYandex)
	resolver := sessions.ResolveByFolder(folderCoords.FolderExtID, models.PermMDBAllRead)
	_, s, err := sess.Begin(context.Background(), resolver)
	require.NoError(t, err)
	require.Equal(t, folderCoords, s.FolderCoords)
}

func TestBeginWithPrimaryExistsCreatesCloud(t *testing.T) {
	folderCoords := metadb.FolderCoords{
		CloudID:     1,
		CloudExtID:  "cloud1",
		FolderID:    10,
		FolderExtID: "folder1",
	}
	cloudCreated := metadb.Cloud{
		CloudID:    1,
		CloudExtID: "cloud1",
	}

	requestID := "test-req-id"
	ctx := requestid.WithRequestID(context.Background(), requestID)
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderCoords.FolderExtID).Return(folderCoords, nil)
	db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(ctx, nil)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), folderCoords.CloudExtID).Return(metadb.Cloud{}, sqlerrors.ErrNotFound).Times(2)
	db.EXPECT().CreateCloud(gomock.Any(), cloudCreated.CloudExtID, metadb.Resources{}, requestID).Return(cloudCreated, nil)
	db.EXPECT().DefaultFeatureFlags(gomock.Any()).Return([]string{}, nil)
	rmClient := resmanMock.NewMockClient(ctrl)
	rmClient.EXPECT().PermissionStages(gomock.Any(), folderCoords.CloudExtID).Return([]string{}, nil)
	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authenticate(gomock.Any(), models.Permission(models.PermMDBAllRead), gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{}, nil)
	sess := NewSessions(db, auth, rmClient, quota.Resources{}, environment.ResourceModelYandex)
	resolver := sessions.ResolveByFolder(folderCoords.FolderExtID, models.PermMDBAllRead)

	_, s, err := sess.Begin(ctx, resolver, sessions.WithPrimary())
	require.NoError(t, err)
	require.Equal(t, folderCoords, s.FolderCoords)
}

func TestBeginWithPreferStandbyDoesNotCreateCloud(t *testing.T) {
	folderCoords := metadb.FolderCoords{
		CloudID:     1,
		CloudExtID:  "cloud1",
		FolderID:    10,
		FolderExtID: "folder1",
	}

	requestID := "test-req-id"
	ctx := requestid.WithRequestID(context.Background(), requestID)
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderCoords.FolderExtID).Return(folderCoords, nil)
	db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(ctx, nil)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), folderCoords.CloudExtID).Return(metadb.Cloud{}, sqlerrors.ErrNotFound).Times(2)
	db.EXPECT().DefaultFeatureFlags(gomock.Any()).Return([]string{}, nil)
	rmClient := resmanMock.NewMockClient(ctrl)
	rmClient.EXPECT().PermissionStages(gomock.Any(), folderCoords.CloudExtID).Return([]string{}, nil)
	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authenticate(gomock.Any(), models.Permission(models.PermMDBAllRead), gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{}, nil)
	sess := NewSessions(db, auth, rmClient, quota.Resources{}, environment.ResourceModelYandex)
	resolver := sessions.ResolveByFolder(folderCoords.FolderExtID, models.PermMDBAllRead)

	_, s, err := sess.Begin(ctx, resolver, sessions.WithPreferStandby())
	require.NoError(t, err)
	require.Equal(t, folderCoords, s.FolderCoords)
}

func TestBeginWithFolderExtIDHalfEmpty(t *testing.T) {
	folderCoordsNotFound := metadb.FolderCoords{
		CloudID:     notFoundID,
		CloudExtID:  "cloud2",
		FolderID:    notFoundID,
		FolderExtID: "folder2",
	}
	cloud := metadb.Cloud{
		CloudExtID: "cloud2",
		CloudID:    notFoundID,
	}
	resolvedFolder := []resmanager.ResolvedFolder{{
		CloudExtID:  folderCoordsNotFound.CloudExtID,
		FolderExtID: folderCoordsNotFound.FolderExtID,
	}}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderCoordsNotFound.FolderExtID).Return(metadb.FolderCoords{}, sqlerrors.ErrNotFound)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), folderCoordsNotFound.CloudExtID).Return(cloud, nil).Times(2)
	db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
	resmanClient := resmanMock.NewMockClient(ctrl)
	resmanClient.EXPECT().ResolveFolders(gomock.Any(), []string{folderCoordsNotFound.FolderExtID}).Return(resolvedFolder, nil)
	resmanClient.EXPECT().PermissionStages(gomock.Any(), folderCoordsNotFound.CloudExtID).Return([]string{folderCoordsNotFound.CloudExtID}, nil)
	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authenticate(gomock.Any(), models.PermMDBAllRead, gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{}, nil)
	sess := NewSessions(db, auth, resmanClient, quota.Resources{}, environment.ResourceModelYandex)
	resolver := sessions.ResolveByFolder(folderCoordsNotFound.FolderExtID, models.PermMDBAllRead)

	_, s, err := sess.Begin(context.Background(), resolver)
	require.NoError(t, err)
	require.Equal(t, folderCoordsNotFound, s.FolderCoords)
}

func TestBeginWithFolderExtIDCreate(t *testing.T) {
	cloud := metadb.Cloud{
		CloudID:    2,
		CloudExtID: "cloud2",
	}
	folderCoordsCreated := metadb.FolderCoords{
		CloudID:     2,
		CloudExtID:  "cloud2",
		FolderID:    20,
		FolderExtID: "folder2",
	}
	resolvedFolder := []resmanager.ResolvedFolder{{
		CloudExtID:  folderCoordsCreated.CloudExtID,
		FolderExtID: folderCoordsCreated.FolderExtID,
	}}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderCoordsCreated.FolderExtID).Return(metadb.FolderCoords{}, sqlerrors.ErrNotFound).Times(2)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), folderCoordsCreated.CloudExtID).Return(cloud, nil).Times(2)
	db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
	db.EXPECT().CreateFolder(gomock.Any(), folderCoordsCreated.FolderExtID, folderCoordsCreated.CloudID).Return(folderCoordsCreated, nil)
	resmanClient := resmanMock.NewMockClient(ctrl)
	resmanClient.EXPECT().ResolveFolders(gomock.Any(), []string{folderCoordsCreated.FolderExtID}).Return(resolvedFolder, nil).Times(2)
	resmanClient.EXPECT().PermissionStages(gomock.Any(), cloud.CloudExtID).Return([]string{}, nil)

	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authenticate(gomock.Any(), models.Permission(models.PermMDBAllCreate), gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{}, nil)
	db.EXPECT().LockCloud(gomock.Any(), cloud.CloudID)
	sess := NewSessions(db, auth, resmanClient, quota.Resources{}, environment.ResourceModelYandex)
	resolver := sessions.ResolveByFolder(folderCoordsCreated.FolderExtID, models.PermMDBAllCreate)

	_, s, err := sess.Begin(context.Background(), resolver)
	require.NoError(t, err)
	require.Equal(t, folderCoordsCreated, s.FolderCoords)
}

func TestBeginWithFolderExtIDCreateError(t *testing.T) {
	cloud := metadb.Cloud{
		CloudID:    2,
		CloudExtID: "cloud2",
	}
	folderCoordsCreated := metadb.FolderCoords{
		CloudID:     2,
		CloudExtID:  "cloud2",
		FolderID:    20,
		FolderExtID: "folder2",
	}
	resolvedFolder := []resmanager.ResolvedFolder{{
		CloudExtID:  folderCoordsCreated.CloudExtID,
		FolderExtID: folderCoordsCreated.FolderExtID,
	}}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderCoordsCreated.FolderExtID).Return(metadb.FolderCoords{}, sqlerrors.ErrNotFound).Times(2)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), folderCoordsCreated.CloudExtID).Return(cloud, nil)
	ctx := context.Background()
	db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(ctx, nil)
	// This is the key expectation - without it we will leak transaction
	db.EXPECT().Rollback(gomock.Any()).Return(nil)

	errCreate := xerrors.New("create error")
	db.EXPECT().CreateFolder(gomock.Any(), folderCoordsCreated.FolderExtID, folderCoordsCreated.CloudID).Return(metadb.FolderCoords{}, errCreate)
	resmanClient := resmanMock.NewMockClient(ctrl)
	resmanClient.EXPECT().ResolveFolders(gomock.Any(), []string{folderCoordsCreated.FolderExtID}).Return(resolvedFolder, nil).Times(2)
	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authenticate(gomock.Any(), models.Permission(models.PermMDBAllCreate), gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{}, nil)
	sess := NewSessions(db, auth, resmanClient, quota.Resources{}, environment.ResourceModelYandex)
	resolver := sessions.ResolveByFolder(folderCoordsCreated.FolderExtID, models.PermMDBAllCreate)

	_, s, err := sess.Begin(context.Background(), resolver)
	require.Error(t, err)
	require.True(t, xerrors.Is(err, errCreate))
	require.NotEqual(t, folderCoordsCreated, s.FolderCoords)
}

func TestBeginWithFolderExtIDCreatePanic(t *testing.T) {
	cloud := metadb.Cloud{
		CloudID:    2,
		CloudExtID: "cloud2",
	}
	folderCoordsCreated := metadb.FolderCoords{
		CloudID:     2,
		CloudExtID:  "cloud2",
		FolderID:    20,
		FolderExtID: "folder2",
	}
	resolvedFolder := []resmanager.ResolvedFolder{{
		CloudExtID:  folderCoordsCreated.CloudExtID,
		FolderExtID: folderCoordsCreated.FolderExtID,
	}}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderCoordsCreated.FolderExtID).Return(metadb.FolderCoords{}, sqlerrors.ErrNotFound).Times(2)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), folderCoordsCreated.CloudExtID).Return(cloud, nil)
	txCtx := withTx(context.Background())
	db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(txCtx, nil)
	// This is the key expectation - without it we will leak transaction
	db.EXPECT().Rollback(gomock.Any()).DoAndReturn(func(ctx context.Context) error {
		assert.True(t, hasTx(ctx), "context doesn't contain tx")
		return nil
	}).MinTimes(1).MaxTimes(42) // Expect that Rollback called at least once

	db.EXPECT().CreateFolder(gomock.Any(), folderCoordsCreated.FolderExtID, folderCoordsCreated.CloudID).
		Do(func(_, _, _ interface{}) { panic("hello") })
	resmanClient := resmanMock.NewMockClient(ctrl)
	resmanClient.EXPECT().ResolveFolders(gomock.Any(), []string{folderCoordsCreated.FolderExtID}).Return(resolvedFolder, nil).Times(2)
	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authenticate(gomock.Any(), models.Permission(models.PermMDBAllCreate), gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{}, nil)
	sess := NewSessions(db, auth, resmanClient, quota.Resources{}, environment.ResourceModelYandex)
	resolver := sessions.ResolveByFolder(folderCoordsCreated.FolderExtID, models.PermMDBAllCreate)

	require.Panics(
		t,
		func() {
			_, _, _ = sess.Begin(context.Background(), resolver)
		},
	)
}

func TestBeginLoadPermissionsFromMetaDB(t *testing.T) {
	folderCoords := metadb.FolderCoords{
		CloudID:     1,
		CloudExtID:  "cloud1",
		FolderID:    10,
		FolderExtID: "folder1",
	}
	cloud := metadb.Cloud{
		CloudID:      1,
		CloudExtID:   "cloud1",
		FeatureFlags: []string{"MDB_VERY_COOL_FEATURE"},
	}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderCoords.FolderExtID).Return(folderCoords, nil)
	db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), cloud.CloudExtID).Return(cloud, nil)
	rmClient := resmanMock.NewMockClient(ctrl)
	rmClient.EXPECT().PermissionStages(gomock.Any(), gomock.Any()).Return([]string{}, nil)
	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authenticate(gomock.Any(), models.Permission(models.PermMDBAllCreate), gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{}, nil)
	db.EXPECT().LockCloud(gomock.Any(), cloud.CloudID)
	sess := NewSessions(db, auth, rmClient, quota.Resources{}, environment.ResourceModelYandex)
	resolver := sessions.ResolveByFolder(folderCoords.FolderExtID, models.PermMDBAllCreate)
	_, s, err := sess.Begin(context.Background(), resolver)
	require.NoError(t, err)
	require.Equal(t, folderCoords, s.FolderCoords)
	require.True(t, s.FeatureFlags.Has("MDB_VERY_COOL_FEATURE"))
}

func TestBeginLoadPermissionsFromIAM(t *testing.T) {
	folderCoords := metadb.FolderCoords{
		CloudID:     1,
		CloudExtID:  "cloud1",
		FolderID:    10,
		FolderExtID: "folder1",
	}
	cloud := metadb.Cloud{
		CloudID:      1,
		CloudExtID:   "cloud1",
		FeatureFlags: []string{},
	}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderCoords.FolderExtID).Return(folderCoords, nil)
	db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), cloud.CloudExtID).Return(cloud, nil)
	rmClient := resmanMock.NewMockClient(ctrl)
	rmClient.EXPECT().PermissionStages(gomock.Any(), gomock.Any()).Return([]string{"MDB_VERY_COOL_FEATURE_FROM_IAM"}, nil)
	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authenticate(gomock.Any(), models.Permission(models.PermMDBAllCreate), gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{}, nil)
	db.EXPECT().LockCloud(gomock.Any(), cloud.CloudID)
	sess := NewSessions(db, auth, rmClient, quota.Resources{}, environment.ResourceModelYandex)
	resolver := sessions.ResolveByFolder(folderCoords.FolderExtID, models.PermMDBAllCreate)
	_, s, err := sess.Begin(context.Background(), resolver)
	require.NoError(t, err)
	require.Equal(t, folderCoords, s.FolderCoords)
	require.True(t, s.FeatureFlags.Has("MDB_VERY_COOL_FEATURE_FROM_IAM"))
}

func TestBeginLoadPermissionsWithoutFeatureFlag(t *testing.T) {
	folderCoords := metadb.FolderCoords{
		CloudID:     1,
		CloudExtID:  "cloud1",
		FolderID:    10,
		FolderExtID: "folder1",
	}
	cloud := metadb.Cloud{
		CloudID:      1,
		CloudExtID:   "cloud1",
		FeatureFlags: []string{"MDB_HADOOP_CLUSTER", "MDB_HADOOP_GPU"},
	}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderCoords.FolderExtID).Return(folderCoords, nil)
	db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), cloud.CloudExtID).Return(cloud, nil)
	rmClient := resmanMock.NewMockClient(ctrl)
	rmClient.EXPECT().PermissionStages(gomock.Any(), cloud.CloudExtID).Return([]string{"MDB_KAFKA_CLUSTER"}, nil)
	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authenticate(gomock.Any(), models.Permission(models.PermMDBAllCreate), gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{}, nil)
	db.EXPECT().LockCloud(gomock.Any(), cloud.CloudID)
	sess := NewSessions(db, auth, rmClient, quota.Resources{}, environment.ResourceModelYandex)
	resolver := sessions.ResolveByFolder(folderCoords.FolderExtID, models.PermMDBAllCreate)
	_, s, err := sess.Begin(context.Background(), resolver)
	require.NoError(t, err)
	require.Equal(t, folderCoords, s.FolderCoords)
	require.False(t, s.FeatureFlags.Has("MDB_ELASTICSEARCH_CLUSTER"))
}

func TestBeginWithClusterIDExists(t *testing.T) {
	clusterID := "cluster1"
	folderCoords := metadb.FolderCoords{
		CloudID:     1,
		CloudExtID:  "cloud1",
		FolderID:    10,
		FolderExtID: "folder1",
	}
	cloud := metadb.Cloud{
		CloudExtID: "cloud1",
		CloudID:    1,
	}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByClusterID(gomock.Any(), clusterID, models.VisibilityVisible).Return(folderCoords, int64(1), clusters.TypeUnknown, nil)
	db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), folderCoords.CloudExtID).Return(cloud, nil)
	rmClient := resmanMock.NewMockClient(ctrl)
	rmClient.EXPECT().PermissionStages(gomock.Any(), folderCoords.CloudExtID).Return([]string{}, nil)
	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authenticate(gomock.Any(), models.Permission(models.PermMDBAllCreate), gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{}, nil)
	db.EXPECT().LockCloud(gomock.Any(), cloud.CloudID)
	sess := NewSessions(db, auth, rmClient, quota.Resources{}, environment.ResourceModelYandex)
	resolver := sessions.ResolveByCluster(clusterID, models.PermMDBAllCreate)
	_, s, err := sess.Begin(context.Background(), resolver)
	require.NoError(t, err)
	require.Equal(t, folderCoords, s.FolderCoords)
}

func TestBeginWithClusterIDNotExists(t *testing.T) {
	clusterID := "cluster1"

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	ctx := context.Background()
	beginCall := db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(ctx, nil)
	fcCall := db.EXPECT().FolderCoordsByClusterID(gomock.Any(), clusterID, models.VisibilityVisible).Return(metadb.FolderCoords{}, int64(0), clusters.TypeUnknown, sqlerrors.ErrNotFound).After(beginCall)
	db.EXPECT().Rollback(ctx).Return(nil).After(fcCall)

	auth := authMock.NewMockAuthenticator(ctrl)
	sess := NewSessions(db, auth, resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)

	resolver := sessions.ResolveByCluster(clusterID, models.PermMDBAllCreate)
	_, _, err := sess.Begin(context.Background(), resolver)
	require.Error(t, err, sqlerrors.ErrNotFound)
}

func TestBeginWithDeletedClusterIDExists(t *testing.T) {
	clusterID := "cluster1"
	folderCoords := metadb.FolderCoords{
		CloudID:     1,
		CloudExtID:  "cloud1",
		FolderID:    10,
		FolderExtID: "folder1",
	}
	cloud := metadb.Cloud{
		CloudExtID: "cloud1",
		CloudID:    1,
	}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByClusterID(gomock.Any(), clusterID, models.VisibilityVisibleOrDeleted).Return(folderCoords, int64(1), clusters.TypeUnknown, nil)
	db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), folderCoords.CloudExtID).Return(cloud, nil)
	rmClient := resmanMock.NewMockClient(ctrl)
	rmClient.EXPECT().PermissionStages(gomock.Any(), folderCoords.CloudExtID).Return([]string{}, nil)
	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authenticate(gomock.Any(), models.Permission(models.PermMDBAllCreate), gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{}, nil)
	db.EXPECT().LockCloud(gomock.Any(), cloud.CloudID)
	sess := NewSessions(db, auth, rmClient, quota.Resources{}, environment.ResourceModelYandex)
	resolver := sessions.ResolveByDeletedCluster(clusterID, models.PermMDBAllCreate)
	_, s, err := sess.Begin(context.Background(), resolver)
	require.NoError(t, err)
	require.Equal(t, folderCoords, s.FolderCoords)
}

func TestBeginWithDeletedClusterIDNotExists(t *testing.T) {
	clusterID := "cluster1"

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	ctx := context.Background()
	beginCall := db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(ctx, nil)
	fcCall := db.EXPECT().FolderCoordsByClusterID(gomock.Any(), clusterID, models.VisibilityVisibleOrDeleted).Return(metadb.FolderCoords{}, int64(0), clusters.TypeUnknown, sqlerrors.ErrNotFound).After(beginCall)
	db.EXPECT().Rollback(ctx).Return(nil).After(fcCall)

	auth := authMock.NewMockAuthenticator(ctrl)
	sess := NewSessions(db, auth, resmanMock.NewMockClient(ctrl), quota.Resources{}, environment.ResourceModelYandex)

	resolver := sessions.ResolveByDeletedCluster(clusterID, models.PermMDBAllCreate)
	_, _, err := sess.Begin(context.Background(), resolver)
	require.Error(t, err, sqlerrors.ErrNotFound)
}

func TestBeginWithMoveClusterDstFolderExists(t *testing.T) {
	clusterID := "cluster1"
	cloud := metadb.Cloud{
		CloudExtID: "cloud1",
		CloudID:    1,
	}
	folderSrcCoords := metadb.FolderCoords{
		CloudID:     cloud.CloudID,
		CloudExtID:  cloud.CloudExtID,
		FolderID:    10,
		FolderExtID: "folder1",
	}
	folderDstCoords := metadb.FolderCoords{
		CloudID:     cloud.CloudID,
		CloudExtID:  cloud.CloudExtID,
		FolderID:    11,
		FolderExtID: "folder2",
	}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByClusterID(gomock.Any(), clusterID, models.VisibilityVisible).Return(folderSrcCoords, int64(1), clusters.TypeUnknown, nil)
	db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
	db.EXPECT().LockCloud(gomock.Any(), folderSrcCoords.CloudID)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), cloud.CloudExtID).Return(cloud, nil)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderDstCoords.FolderExtID).Return(folderDstCoords, nil)
	rmClient := resmanMock.NewMockClient(ctrl)
	rmClient.EXPECT().PermissionStages(gomock.Any(), cloud.CloudExtID).Return([]string{}, nil)
	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authenticate(gomock.Any(), models.Permission(models.PermMDBAllModify), gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{}, nil)
	sess := NewSessions(db, auth, rmClient, quota.Resources{}, environment.ResourceModelYandex)

	resolver := sessions.ResolveByClusterAndDstFolder(clusterID, "folder2")
	_, s, err := sess.Begin(context.Background(), resolver)
	require.NoError(t, err)
	require.NoError(t, err)
	require.Equal(t, folderSrcCoords, s.FolderCoords)
	require.NotNil(t, s.DstFolderCoords)
	require.Equal(t, folderDstCoords, *s.DstFolderCoords)
}

func TestBeginWithMoveClusterDstFolderNotExists(t *testing.T) {
	clusterID := "cluster1"
	cloud := metadb.Cloud{
		CloudExtID: "cloud1",
		CloudID:    1,
	}
	folderSrcCoords := metadb.FolderCoords{
		CloudID:     cloud.CloudID,
		CloudExtID:  cloud.CloudExtID,
		FolderID:    10,
		FolderExtID: "folder1",
	}
	folderDstCoords := metadb.FolderCoords{
		CloudID:     cloud.CloudID,
		CloudExtID:  cloud.CloudExtID,
		FolderID:    11,
		FolderExtID: "folder2",
	}
	resolvedFolder := []resmanager.ResolvedFolder{{
		CloudExtID:  folderDstCoords.CloudExtID,
		FolderExtID: folderDstCoords.FolderExtID,
	}}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	db.EXPECT().FolderCoordsByClusterID(gomock.Any(), clusterID, models.VisibilityVisible).Return(folderSrcCoords, int64(1), clusters.TypeUnknown, nil)
	db.EXPECT().Begin(gomock.Any(), gomock.Any()).Return(context.Background(), nil)
	db.EXPECT().LockCloud(gomock.Any(), folderSrcCoords.CloudID)
	db.EXPECT().CloudByCloudExtID(gomock.Any(), cloud.CloudExtID).Return(cloud, nil).Times(2)
	db.EXPECT().FolderCoordsByFolderExtID(gomock.Any(), folderDstCoords.FolderExtID).Return(metadb.FolderCoords{}, sqlerrors.ErrNotFound).Times(2)
	db.EXPECT().CreateFolder(gomock.Any(), folderDstCoords.FolderExtID, folderDstCoords.CloudID).Return(folderDstCoords, nil)
	rmClient := resmanMock.NewMockClient(ctrl)
	rmClient.EXPECT().ResolveFolders(gomock.Any(), []string{folderDstCoords.FolderExtID}).Return(resolvedFolder, nil)
	rmClient.EXPECT().PermissionStages(gomock.Any(), cloud.CloudExtID).Return([]string{}, nil)
	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authorize(gomock.Any(), gomock.Any(), models.Permission(models.PermMDBAllCreate), gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(nil)
	auth.EXPECT().Authenticate(gomock.Any(), models.Permission(models.PermMDBAllModify), gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{}, nil)

	sess := NewSessions(db, auth, rmClient, quota.Resources{}, environment.ResourceModelYandex)

	resolver := sessions.ResolveByClusterAndDstFolder(clusterID, "folder2")
	_, s, err := sess.Begin(context.Background(), resolver)
	require.NoError(t, err)
	require.NoError(t, err)
	require.Equal(t, folderSrcCoords, s.FolderCoords)
	require.NotNil(t, s.DstFolderCoords)
	require.Equal(t, folderDstCoords, *s.DstFolderCoords)
}

func TestClusterIDOnPrimaryStandby(t *testing.T) {
	clusterID := "cluster1"
	folderCoords := metadb.FolderCoords{
		CloudID:     1,
		CloudExtID:  "cloud1",
		FolderID:    10,
		FolderExtID: "folder1",
	}

	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	db := dbMock.NewMockBackend(ctrl)
	auth := authMock.NewMockAuthenticator(ctrl)
	ctx := context.Background()
	ctx = ctxlog.WithFields(ctx, log.String("cluster.id", clusterID))
	ctxStandby := withStandby(ctx)
	beginStandbyCall := db.EXPECT().Begin(ctx, sqlutil.Standby).Return(ctxStandby, nil)
	fcStandbyCall := db.EXPECT().FolderCoordsByClusterID(ctxStandby, clusterID, models.VisibilityVisible).Return(folderCoords, int64(1), clusters.TypeUnknown, nil).After(beginStandbyCall)
	ctxPrimary := withPrimary(ctx)
	beginPrimaryCall := db.EXPECT().Begin(ctx, sqlutil.Primary).Return(ctxPrimary, nil).After(fcStandbyCall)
	fcPrimaryCall := db.EXPECT().FolderCoordsByClusterID(ctxPrimary, clusterID, models.VisibilityVisible).Return(folderCoords, int64(2), clusters.TypeUnknown, nil).After(beginPrimaryCall)
	ctxPrimary = ctxlog.WithFields(ctxPrimary, log.String("cloud.ext_id", folderCoords.CloudExtID))
	ctxPrimary = ctxlog.WithFields(ctxPrimary, log.String("folder.ext_id", folderCoords.FolderExtID))
	authCall := auth.EXPECT().Authenticate(ctxPrimary, models.PermMDBAllRead, gomock.AssignableToTypeOf(cloudauth.Resource{})).Return(as.Subject{User: &as.UserAccount{ID: "userID"}}, nil).After(fcPrimaryCall)
	ctxPrimary = ctxlog.WithFields(ctxPrimary, log.String("user_type", string(as.AccountTypeUser)), log.String("user_id", "userID"))
	db.EXPECT().CloudByCloudExtID(ctxPrimary, folderCoords.CloudExtID).Return(metadb.Cloud{}, nil).Times(2)
	rmClient := resmanMock.NewMockClient(ctrl)
	rmClient.EXPECT().PermissionStages(gomock.Any(), folderCoords.CloudExtID).Return([]string{}, nil).After(authCall)

	sess := NewSessions(db, auth, rmClient, quota.Resources{}, environment.ResourceModelYandex)
	resolver := sessions.ResolveByCluster(clusterID, models.PermMDBAllRead)
	_, _, err := sess.Begin(context.Background(), resolver)
	require.NoError(t, err)
}
