package grpc_test

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/require"
	"google.golang.org/grpc"

	resmanv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/resourcemanager/v1"
	resmanager "a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	resmanagergrpc "a.yandex-team.ru/cloud/mdb/internal/compute/resmanager/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestAccessBindings(t *testing.T) {
	mocker, l := RunMocker(t, "testdata/test_access_bindings.json")
	defer StopMocker(t, mocker)
	ctx := context.Background()

	client, err := resmanagergrpc.NewClient(
		ctx,
		mocker.Addr(),
		"grpcmocker_test",
		grpcutil.ClientConfig{Security: grpcutil.SecurityConfig{Insecure: true}},
		&grpcutil.PerRPCCredentialsStatic{},
		l,
	)

	require.NoError(t, err)

	bindings, err := client.ListAccessBindings(ctx, "single_page_empty", false)
	require.NoError(t, err)
	require.Empty(t, bindings)

	bindings, err = client.ListAccessBindings(ctx, "single_page_exists", false)
	require.NoError(t, err)
	require.NotEmpty(t, bindings)

	require.Equal(t, resmanager.AccessBinding{
		RoleID: "role1",
		Subject: resmanager.Subject{
			ID:   "subjectId1",
			Type: "subjectType1",
		},
	}, bindings[0])

	bindings, err = client.ListAccessBindings(ctx, "single_page_exists", true)
	require.NoError(t, err)
	require.NotEmpty(t, bindings)

	require.Equal(t, resmanager.AccessBinding{
		RoleID: "role1",
		Subject: resmanager.Subject{
			ID:   "subjectId1",
			Type: "subjectType1",
		},
	}, bindings[0])
	require.Equal(t, resmanager.AccessBinding{
		RoleID: "role1private",
		Subject: resmanager.Subject{
			ID:   "subjectId1",
			Type: "subjectType1",
		},
	}, bindings[1])

	bindings, err = client.ListAccessBindings(ctx, "multiple_page", false)
	require.NoError(t, err)
	require.Len(t, bindings, 2)

	require.Equal(t, resmanager.AccessBinding{
		RoleID: "role1",
		Subject: resmanager.Subject{
			ID:   "subjectId1",
			Type: "subjectType1",
		},
	}, bindings[0])
	require.Equal(t, resmanager.AccessBinding{
		RoleID: "role2",
		Subject: resmanager.Subject{
			ID:   "subjectId2",
			Type: "subjectType2",
		},
	}, bindings[1])
}

func TestCloud(t *testing.T) {
	mocker, l := RunMocker(t, "testdata/test_cloud.json")
	defer StopMocker(t, mocker)

	ctx := context.Background()

	client, err := resmanagergrpc.NewClient(
		ctx,
		mocker.Addr(),
		"grpcmocker_test",
		grpcutil.ClientConfig{Security: grpcutil.SecurityConfig{Insecure: true}},
		&grpcutil.PerRPCCredentialsStatic{},
		l,
	)

	require.NoError(t, err)

	_, err = client.Cloud(ctx, "unknown")
	require.Error(t, err)
	semErr := semerr.AsSemanticError(err)
	require.Error(t, semErr)
	require.Equal(t, semerr.SemanticNotFound, semErr.Semantic)

	cloud1, err := client.Cloud(ctx, "cloud1")
	require.NoError(t, err)
	require.Equal(t, "cloud1", cloud1.CloudID)
	require.Equal(t, "cloud one", cloud1.Name)
	require.Equal(t, resmanager.CloudStatusActive, cloud1.Status)

	cloudBlocked, err := client.Cloud(ctx, "cloudblocked")
	require.NoError(t, err)
	require.Equal(t, "cloudblocked", cloudBlocked.CloudID)
	require.Equal(t, "blocked cloud", cloudBlocked.Name)
	require.Equal(t, resmanager.CloudStatusBlocked, cloudBlocked.Status)
}

func RunMocker(t *testing.T, cfg string) (*grpcmocker.Server, log.Logger) {
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)

	server, err := grpcmocker.Run(
		":0",
		func(s *grpc.Server) error {
			resmanv1.RegisterCloudServiceServer(s, &resmanv1.UnimplementedCloudServiceServer{})
			return nil
		},
		grpcmocker.WithLogger(l),
		grpcmocker.WithExpectationsFromFile(cfg),
	)
	require.NoError(t, err)
	require.NotNil(t, server)
	return server, l
}

func StopMocker(_ *testing.T, server *grpcmocker.Server) {
	_ = server.Stop(5 * time.Second)
}
