package basic

import (
	"context"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/dataplatform/api/connman"
	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	"a.yandex-team.ru/transfer_manager/go/pkg/cleanup"
)

func TestBasicConnectionProvider(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	recipe, err := NewConnmanRecipe()
	require.NoError(t, err)
	defer cleanup.Close(recipe, logger.Log)

	client := recipe.Client

	defer func() {
		list, err := client.List(ctx, &connman.ListConnectionRequest{FolderId: "allowed-folder"})
		require.NoError(t, err)
		for _, row := range list.Connection {
			op, err := client.Delete(ctx, &connman.DeleteConnectionRequest{Id: row.Id})
			require.NoError(t, err)
			require.True(t, op.Done)
		}
	}()

	res := &connman.Connection{}
	t.Run("create", func(t *testing.T) {
		op, err := client.Create(ctx, &connman.CreateConnectionRequest{
			FolderId: "allowed-folder",
			Name:     "simple-mdb-pg-conn",
			Params: &connman.ConnectionParams{Type: &connman.ConnectionParams_Postgresql{Postgresql: &connman.PostgreSQLConnection{
				Connection: &connman.PostgreSQLConnection_ManagedClusterId{ManagedClusterId: "cluster-id"},
				Auth: &connman.PostgreSQLAuth{Security: &connman.PostgreSQLAuth_UserPassword{UserPassword: &connman.UserPasswordAuth{
					User:     "foo-user",
					Password: &connman.Password{Value: &connman.Password_Raw{Raw: "MyVerySecurePassword123@#!"}},
				}}},
			}}},
		})
		require.NoError(t, err)
		require.True(t, op.Done)
		require.NoError(t, op.GetResponse().UnmarshalTo(res))
		require.NotNil(t, res)
		require.Equal(t, res.Name, "simple-mdb-pg-conn")
		createdConn, err := client.Get(ctx, &connman.GetConnectionRequest{Id: res.Id})
		require.NoError(t, err)
		require.Equal(t, createdConn.GetParams().GetPostgresql().GetManagedClusterId(), "cluster-id")
	})
	t.Run("list-conns-basic", func(t *testing.T) {
		list, err := client.List(ctx, &connman.ListConnectionRequest{FolderId: "allowed-folder", View: connman.ConnectionView_CONNECTION_VIEW_BASIC})
		require.NoError(t, err)
		require.Len(t, list.Connection, 1)
		conn := list.Connection[0]
		pgParams, ok := conn.Params.Type.(*connman.ConnectionParams_Postgresql)
		require.True(t, ok)
		require.NotNil(t, pgParams.Postgresql.Auth.Security)
		userPass, ok := pgParams.Postgresql.Auth.Security.(*connman.PostgreSQLAuth_UserPassword)
		require.True(t, ok)
		require.Nil(t, userPass.UserPassword.Password.Value)
	})
	t.Run("list-conns-sensitive", func(t *testing.T) {
		list, err := client.List(ctx, &connman.ListConnectionRequest{FolderId: "allowed-folder", View: connman.ConnectionView_CONNECTION_VIEW_SENSITIVE})
		require.NoError(t, err)
		require.Len(t, list.Connection, 1)
		conn := list.Connection[0]
		pgParams, ok := conn.Params.Type.(*connman.ConnectionParams_Postgresql)
		require.True(t, ok)
		require.NotNil(t, pgParams.Postgresql.Auth.Security)
		userPass, ok := pgParams.Postgresql.Auth.Security.(*connman.PostgreSQLAuth_UserPassword)
		require.True(t, ok)
		pass, ok := userPass.UserPassword.Password.Value.(*connman.Password_Raw)
		require.True(t, ok)
		require.Equal(t, pass.Raw, "MyVerySecurePassword123@#!")
	})
	t.Run("delete", func(t *testing.T) {
		list, err := client.List(ctx, &connman.ListConnectionRequest{FolderId: "allowed-folder", View: connman.ConnectionView_CONNECTION_VIEW_SENSITIVE})
		require.NoError(t, err)
		require.Len(t, list.Connection, 1)
		conn := list.Connection[0]
		op, err := client.Delete(ctx, &connman.DeleteConnectionRequest{Id: conn.Id})
		require.NoError(t, err)
		require.True(t, op.Done)
		_, err = client.Get(ctx, &connman.GetConnectionRequest{Id: conn.Id})
		require.Error(t, err)
	})
}
