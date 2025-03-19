package auth

import (
	"context"
	"fmt"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/require"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	grpc_metadata "google.golang.org/grpc/metadata"
	"log"
	"net"
	"strconv"
	"sync"
	"testing"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1"
	auth_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/auth/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/library/go/test/yatest"
)

////////////////////////////////////////////////////////////////////////////////

func getCertFilePath() string {
	return yatest.SourcePath("cloud/blockstore/tests/certs/server.crt")
}

func getCertPrivateKeyFilePath() string {
	return yatest.SourcePath("cloud/blockstore/tests/certs/server.key")
}

////////////////////////////////////////////////////////////////////////////////

func getPort(l net.Listener) (uint32, error) {
	_, portStr, err := net.SplitHostPort(l.Addr().String())
	if err != nil {
		return 0, fmt.Errorf(
			"failed to parse port out of addr %v: %w",
			l.Addr().String(),
			err,
		)
	}

	port, err := strconv.Atoi(portStr)
	if err != nil {
		return 0, fmt.Errorf(
			"failed to parse port '%v' as an int: %w",
			portStr,
			err,
		)
	}

	return uint32(port), nil
}

////////////////////////////////////////////////////////////////////////////////

type mockAccessService struct {
	mock.Mock
}

func (s *mockAccessService) Authenticate(
	ctx context.Context,
	req *servicecontrol.AuthenticateRequest,
) (*servicecontrol.AuthenticateResponse, error) {

	args := s.Called(ctx, req)
	res, _ := args.Get(0).(*servicecontrol.AuthenticateResponse)
	return res, args.Error(1)
}

func (s *mockAccessService) Authorize(
	ctx context.Context,
	req *servicecontrol.AuthorizeRequest,
) (*servicecontrol.AuthorizeResponse, error) {

	args := s.Called(ctx, req)
	res, _ := args.Get(0).(*servicecontrol.AuthorizeResponse)
	return res, args.Error(1)
}

func (s *mockAccessService) BulkAuthorize(
	ctx context.Context,
	req *servicecontrol.BulkAuthorizeRequest,
) (*servicecontrol.BulkAuthorizeResponse, error) {

	args := s.Called(ctx, req)
	res, _ := args.Get(0).(*servicecontrol.BulkAuthorizeResponse)
	return res, args.Error(1)
}

////////////////////////////////////////////////////////////////////////////////

func runMockAccessService() (*mockAccessService, uint32, func(), error) {
	service := &mockAccessService{}

	creds, err := credentials.NewServerTLSFromFile(
		getCertFilePath(),
		getCertPrivateKeyFilePath(),
	)
	if err != nil {
		return nil, 0, nil, fmt.Errorf("failed to create creds: %w", err)
	}

	server := grpc.NewServer(
		grpc.Creds(creds),
	)
	servicecontrol.RegisterAccessServiceServer(server, service)

	lis, err := net.Listen("tcp", ":0")
	if err != nil {
		return nil, 0, nil, fmt.Errorf("failed to listen: %w", err)
	}

	port, err := getPort(lis)
	if err != nil {
		return nil, 0, nil, err
	}

	log.Printf("Running mockAccessService on port %v", port)

	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		defer wg.Done()

		err := server.Serve(lis)
		if err != nil && err != grpc.ErrServerStopped {
			log.Fatalf("Error serving grpc: %v", err)
		}
	}()

	return service, port, func() {
		server.Stop()
		wg.Wait()
	}, nil
}

////////////////////////////////////////////////////////////////////////////////

func containsHeaders(
	t *testing.T,
	headers map[string]string,
) func(context.Context) bool {

	return func(ctx context.Context) bool {
		md, ok := grpc_metadata.FromIncomingContext(ctx)
		if !ok {
			return assert.ElementsMatch(t, map[string]string{}, headers)
		}
		for k, v := range headers {
			ok = assert.Equal(t, []string{v}, md[k]) && ok
		}

		return ok
	}
}

////////////////////////////////////////////////////////////////////////////////

func createAccessServiceClient(
	disableAuthorization bool,
	port uint32,
	folderID string,
) (AccessServiceClient, error) {
	accessServiceEndpoint := fmt.Sprintf("localhost:%v", port)
	certFilePath := getCertFilePath()
	connectionTimeout := "1s"
	retryCount := uint32(1)
	perRetryTimeout := "100ms"
	keepAliveTime := "1s"
	keepAliveTimeout := "3s"
	authorizationCacheLifetime := "100s"

	return NewAccessServiceClient(&auth_config.AuthConfig{
		DisableAuthorization:       &disableAuthorization,
		AccessServiceEndpoint:      &accessServiceEndpoint,
		CertFile:                   &certFilePath,
		ConnectionTimeout:          &connectionTimeout,
		RetryCount:                 &retryCount,
		PerRetryTimeout:            &perRetryTimeout,
		KeepAliveTime:              &keepAliveTime,
		KeepAliveTimeout:           &keepAliveTimeout,
		FolderId:                   &folderID,
		AuthorizationCacheLifetime: &authorizationCacheLifetime,
	})
}

////////////////////////////////////////////////////////////////////////////////

func TestDisabledAuthorization(t *testing.T) {
	srv, port, closeServer, err := runMockAccessService()
	require.NoError(t, err)
	defer closeServer()

	client, err := createAccessServiceClient(true, port, "TestFolderId")
	require.NoError(t, err)

	ctx := headers.SetIncomingRequestID(context.Background(), "TestRequestId")

	subj, err := client.Authorize(ctx, "TestPermission")
	mock.AssertExpectationsForObjects(t, srv)
	assert.NoError(t, err)
	assert.Equal(t, "unauthorized", subj)
}

func TestAuthorizeUser(t *testing.T) {
	srv, port, closeServer, err := runMockAccessService()
	require.NoError(t, err)
	defer closeServer()

	client, err := createAccessServiceClient(false, port, "TestFolderId")
	require.NoError(t, err)

	ctx := headers.SetIncomingAccessToken(
		headers.SetIncomingRequestID(context.Background(), "TestRequestId"),
		"TestToken",
	)

	srv.On(
		"Authorize",
		mock.MatchedBy(containsHeaders(t, map[string]string{
			"x-request-id": "TestRequestId",
		})),
		&servicecontrol.AuthorizeRequest{
			Identity: &servicecontrol.AuthorizeRequest_IamToken{
				IamToken: "TestToken",
			},
			Permission: "TestPermission",
			ResourcePath: []*servicecontrol.Resource{
				&servicecontrol.Resource{
					Id:   "TestFolderId",
					Type: "resource-manager.folder",
				},
			},
		}).Times(1).Return(&servicecontrol.AuthorizeResponse{
		Subject: &servicecontrol.Subject{
			Type: &servicecontrol.Subject_UserAccount_{
				UserAccount: &servicecontrol.Subject_UserAccount{
					Id: "TestUser",
				},
			},
		},
	}, nil)

	subj, err := client.Authorize(ctx, "TestPermission")
	mock.AssertExpectationsForObjects(t, srv)
	assert.NoError(t, err)
	assert.Equal(t, "user-TestUser", subj)
}

func TestCacheForAuthorizationStoring(t *testing.T) {
	srv, port, closeServer, err := runMockAccessService()
	require.NoError(t, err)
	defer closeServer()

	client, err := createAccessServiceClient(false, port, "TestFolderId")
	require.NoError(t, err)

	ctx := headers.SetIncomingAccessToken(
		headers.SetIncomingRequestID(context.Background(), "TestRequestId"),
		"TestToken",
	)

	srv.On(
		"Authorize",
		mock.MatchedBy(containsHeaders(t, map[string]string{
			"x-request-id": "TestRequestId",
		})),
		&servicecontrol.AuthorizeRequest{
			Identity: &servicecontrol.AuthorizeRequest_IamToken{
				IamToken: "TestToken",
			},
			Permission: "TestPermission",
			ResourcePath: []*servicecontrol.Resource{
				&servicecontrol.Resource{
					Id:   "TestFolderId",
					Type: "resource-manager.folder",
				},
			},
		}).Times(1).Return(&servicecontrol.AuthorizeResponse{
		Subject: &servicecontrol.Subject{
			Type: &servicecontrol.Subject_UserAccount_{
				UserAccount: &servicecontrol.Subject_UserAccount{
					Id: "TestUser",
				},
			},
		},
	}, nil)

	// call authorize first time to set data in the cache
	subj, err := client.Authorize(ctx, "TestPermission")
	mock.AssertExpectationsForObjects(t, srv)
	assert.NoError(t, err)
	assert.Equal(t, "user-TestUser", subj)

	srv.On(
		"Authorize",
		mock.MatchedBy(containsHeaders(t, map[string]string{
			"x-request-id": "TestRequestId",
		})),
		&servicecontrol.AuthorizeRequest{
			Identity: &servicecontrol.AuthorizeRequest_IamToken{
				IamToken: "TestToken",
			},
			Permission: "TestPermission",
			ResourcePath: []*servicecontrol.Resource{
				&servicecontrol.Resource{
					Id:   "TestFolderId",
					Type: "resource-manager.folder",
				},
			},
		}).Times(0).Return(nil, fmt.Errorf("Authorize error"))

	// even if IAM should return "authorize error", we shouldn't get an error,
	// because there is a good authorization in the client cache
	subj, err = client.Authorize(ctx, "TestPermission")
	mock.AssertExpectationsForObjects(t, srv)
	assert.NoError(t, err)
	assert.Equal(t, "user-TestUser", subj)
}

func TestCacheForAuthorizationDifferentRequests(t *testing.T) {
	srv, port, closeServer, err := runMockAccessService()
	require.NoError(t, err)
	defer closeServer()

	client, err := createAccessServiceClient(false, port, "TestFolderId")
	require.NoError(t, err)

	ctx := headers.SetIncomingAccessToken(
		headers.SetIncomingRequestID(context.Background(), "TestRequestId"),
		"TestToken",
	)

	srv.On(
		"Authorize",
		mock.MatchedBy(containsHeaders(t, map[string]string{
			"x-request-id": "TestRequestId",
		})),
		&servicecontrol.AuthorizeRequest{
			Identity: &servicecontrol.AuthorizeRequest_IamToken{
				IamToken: "TestToken",
			},
			Permission: "TestPermission",
			ResourcePath: []*servicecontrol.Resource{
				&servicecontrol.Resource{
					Id:   "TestFolderId",
					Type: "resource-manager.folder",
				},
			},
		}).Times(1).Return(&servicecontrol.AuthorizeResponse{
		Subject: &servicecontrol.Subject{
			Type: &servicecontrol.Subject_UserAccount_{
				UserAccount: &servicecontrol.Subject_UserAccount{
					Id: "TestUser",
				},
			},
		},
	}, nil)

	subj, err := client.Authorize(ctx, "TestPermission")
	mock.AssertExpectationsForObjects(t, srv)
	assert.NoError(t, err)
	assert.Equal(t, "user-TestUser", subj)

	srv.On(
		"Authorize",
		mock.MatchedBy(containsHeaders(t, map[string]string{
			"x-request-id": "TestRequestId",
		})),
		&servicecontrol.AuthorizeRequest{
			Identity: &servicecontrol.AuthorizeRequest_IamToken{
				IamToken: "TestToken",
			},
			Permission: "TestAnotherPermission",
			ResourcePath: []*servicecontrol.Resource{
				&servicecontrol.Resource{
					Id:   "TestFolderId",
					Type: "resource-manager.folder",
				},
			},
		}).Times(1).Return(&servicecontrol.AuthorizeResponse{
		Subject: &servicecontrol.Subject{
			Type: &servicecontrol.Subject_UserAccount_{
				UserAccount: &servicecontrol.Subject_UserAccount{
					Id: "AnotherTestUser",
				},
			},
		},
	}, nil)

	subj, err = client.Authorize(ctx, "TestAnotherPermission")
	mock.AssertExpectationsForObjects(t, srv)
	assert.NoError(t, err)
	assert.Equal(t, "user-AnotherTestUser", subj)
}

func TestAuthorizeService(t *testing.T) {
	srv, port, closeServer, err := runMockAccessService()
	require.NoError(t, err)
	defer closeServer()

	client, err := createAccessServiceClient(false, port, "TestFolderId")
	require.NoError(t, err)

	ctx := headers.SetIncomingAccessToken(
		headers.SetIncomingRequestID(context.Background(), "TestRequestId"),
		"TestToken",
	)

	srv.On(
		"Authorize",
		mock.MatchedBy(containsHeaders(t, map[string]string{
			"x-request-id": "TestRequestId",
		})),
		&servicecontrol.AuthorizeRequest{
			Identity: &servicecontrol.AuthorizeRequest_IamToken{
				IamToken: "TestToken",
			},
			Permission: "TestPermission",
			ResourcePath: []*servicecontrol.Resource{
				&servicecontrol.Resource{
					Id:   "TestFolderId",
					Type: "resource-manager.folder",
				},
			},
		}).Times(1).Return(&servicecontrol.AuthorizeResponse{
		Subject: &servicecontrol.Subject{
			Type: &servicecontrol.Subject_ServiceAccount_{
				ServiceAccount: &servicecontrol.Subject_ServiceAccount{
					Id:       "TestService",
					FolderId: "TestServiceFolderId",
				},
			},
		},
	}, nil)

	subj, err := client.Authorize(ctx, "TestPermission")
	mock.AssertExpectationsForObjects(t, srv)
	assert.NoError(t, err)
	assert.Equal(t, "svc-TestService-TestServiceFolderId", subj)
}

func TestDoNotAuthorizeAnonymous(t *testing.T) {
	srv, port, closeServer, err := runMockAccessService()
	require.NoError(t, err)
	defer closeServer()

	client, err := createAccessServiceClient(false, port, "TestFolderId")
	require.NoError(t, err)

	ctx := headers.SetIncomingAccessToken(
		headers.SetIncomingRequestID(context.Background(), "TestRequestId"),
		"TestToken",
	)

	srv.On(
		"Authorize",
		mock.MatchedBy(containsHeaders(t, map[string]string{
			"x-request-id": "TestRequestId",
		})),
		&servicecontrol.AuthorizeRequest{
			Identity: &servicecontrol.AuthorizeRequest_IamToken{
				IamToken: "TestToken",
			},
			Permission: "TestPermission",
			ResourcePath: []*servicecontrol.Resource{
				&servicecontrol.Resource{
					Id:   "TestFolderId",
					Type: "resource-manager.folder",
				},
			},
		}).Times(1).Return(&servicecontrol.AuthorizeResponse{
		Subject: &servicecontrol.Subject{
			Type: &servicecontrol.Subject_AnonymousAccount_{
				AnonymousAccount: &servicecontrol.Subject_AnonymousAccount{},
			},
		},
	}, nil)

	subj, err := client.Authorize(ctx, "TestPermission")
	mock.AssertExpectationsForObjects(t, srv)
	assert.Error(t, err)
	assert.Equal(t, "", subj)
}

func TestDoNotAuthorizeWithoutToken(t *testing.T) {
	srv, port, closeServer, err := runMockAccessService()
	require.NoError(t, err)
	defer closeServer()

	client, err := createAccessServiceClient(false, port, "TestFolderId")
	require.NoError(t, err)

	ctx := headers.SetIncomingRequestID(context.Background(), "TestRequestId")

	subj, err := client.Authorize(ctx, "TestPermission")
	mock.AssertExpectationsForObjects(t, srv)
	assert.Error(t, err)
	assert.Equal(t, "", subj)
}

func TestDoNotAuthorize(t *testing.T) {
	srv, port, closeServer, err := runMockAccessService()
	require.NoError(t, err)
	defer closeServer()

	client, err := createAccessServiceClient(false, port, "TestFolderId")
	require.NoError(t, err)

	ctx := headers.SetIncomingAccessToken(
		headers.SetIncomingRequestID(context.Background(), "TestRequestId"),
		"TestToken",
	)

	srv.On(
		"Authorize",
		mock.MatchedBy(containsHeaders(t, map[string]string{
			"x-request-id": "TestRequestId",
		})),
		&servicecontrol.AuthorizeRequest{
			Identity: &servicecontrol.AuthorizeRequest_IamToken{
				IamToken: "TestToken",
			},
			Permission: "TestPermission",
			ResourcePath: []*servicecontrol.Resource{
				&servicecontrol.Resource{
					Id:   "TestFolderId",
					Type: "resource-manager.folder",
				},
			},
		}).Times(1).Return(nil, fmt.Errorf("Authorize error"))

	subj, err := client.Authorize(ctx, "TestPermission")
	mock.AssertExpectationsForObjects(t, srv)
	assert.Error(t, err)
	assert.Equal(t, "", subj)
}
