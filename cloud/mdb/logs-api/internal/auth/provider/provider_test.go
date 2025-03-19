package provider

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"
	"golang.yandex/hasql"

	"a.yandex-team.ru/cloud/mdb/internal/auth"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/compute/accessservice/stub"
	metadblogs "a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestAuthorizeProvider(t *testing.T) {
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		panic(err)
	}

	asMock := stub.AccessService{Mapping: map[string]stub.Data{
		"r-token": {
			Subject: as.Subject{User: &as.UserAccount{ID: "read-user"}},
			Folders: map[string]stub.PermissionSet{
				"project1": {ReadOnly: true},
				"project2": {ReadOnly: true},
			},
			Clouds: map[string]struct{}{
				"project1": {},
				"project2": {},
			},
			ServiceAccounts: map[string]struct{}{},
		},
	}}

	ctrl := gomock.NewController(t)
	metadb := metadblogs.NewMockMetaDB(ctrl)
	metadb.EXPECT().Begin(gomock.Any(), gomock.Any()).DoAndReturn(func(ctx context.Context, ns hasql.NodeStateCriteria) (context.Context, error) {
		return ctx, nil
	}).AnyTimes()
	metadb.EXPECT().Rollback(gomock.Any()).Return(nil).AnyTimes()
	metadb.EXPECT().FolderExtIDByClusterID(gomock.Any(), gomock.Any(), gomock.Any()).DoAndReturn(func(ctx context.Context, cid string, _ interface{}) (string, error) {
		mapping := map[string]string{
			"ch": "project1",
			"kf": "project2",
		}
		if v, ok := mapping[cid]; ok {
			return v, nil
		}
		return "", xerrors.Errorf("not found %q", cid)
	}).AnyTimes()

	authProvider := NewAuthProvider(metadb, &asMock, nil, l)
	ctx := context.Background()

	t.Run("Auth valid clusters", func(t *testing.T) {
		ctx := auth.WithAuthToken(ctx, "r-token")
		require.NoError(t, authProvider.Authorize(ctx, []models.LogSource{
			{Type: models.LogSourceTypeClickhouse, ID: "ch"},
			{Type: models.LogSourceTypeKafka, ID: "kf"},
		}))
	})

	t.Run("Auth failed without token", func(t *testing.T) {
		require.EqualError(t, authProvider.Authorize(ctx, []models.LogSource{
			{Type: models.LogSourceTypeClickhouse, ID: "ch"},
			{Type: models.LogSourceTypeKafka, ID: "kf"},
		}), "missing auth token")
	})

	t.Run("Auth failed for not found clusters", func(t *testing.T) {
		ctx := auth.WithAuthToken(ctx, "r-token")
		require.EqualError(t, authProvider.Authorize(ctx, []models.LogSource{
			{Type: models.LogSourceTypeClickhouse, ID: "ch"},
			{Type: models.LogSourceTypeKafka, ID: "kf_invalid"},
		}), "not found \"kf_invalid\"")
	})
}
