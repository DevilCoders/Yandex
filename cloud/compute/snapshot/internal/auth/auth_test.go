package auth

import (
	"context"
	"testing"

	"cuelang.org/go/pkg/strings"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/compute/go-common/pkg/xrequestid"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/mock"
	"go.uber.org/zap"
	"go.uber.org/zap/zaptest"
	grpc_metadata "google.golang.org/grpc/metadata"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1"
	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	asmock "a.yandex-team.ru/cloud/iam/accessservice/mock/go"
)

func TestAccessServiceClient(t *testing.T) {
	at := assert.New(t)

	logger := zaptest.NewLogger(t, zaptest.WrapOptions(zap.Development()))
	ctx := context.Background()
	ctx = ctxlog.WithLogger(ctx, logger)

	asMock, address, asMockCancel, err := asmock.NewMockAccessService(&asmock.Options{LogFunc: t.Logf})
	if err != nil {
		t.Fatalf("Can't start access service mock: %v", err)
	}
	defer asMockCancel()

	testFolder := "testFolderID"
	config := Config{Address: address, InsecureConnection: true, DenyUnauthorizedRequests: true, FolderID: testFolder}
	c, err := NewAccessServiceClient(ctx, config)
	if err != nil {
		t.Fatalf("Can't create access client: %v", err)
	}

	testReqID := "TestRequestId"
	testTokenOK := "testToken"
	testTokenBad := "testTokenBad"
	testTokenBadPrefix := "testBadPrefix"
	testPermission := "testPermission"
	testUser := "testUser"
	testErr := xerrors.New("testErr")

	addPrefix := func(s string) string {
		return authTokenPrefixGRPC + s
	}

	// OK
	asMock.On(
		"Authorize",
		mock.MatchedBy(asmock.ContainsGRPCMetadataStrings(t,
			requestIDHeader, testReqID,
		)),
		&servicecontrol.AuthorizeRequest{
			Identity: &servicecontrol.AuthorizeRequest_IamToken{
				IamToken: testTokenOK,
			},
			Permission: testPermission,
			ResourcePath: []*servicecontrol.Resource{
				&servicecontrol.Resource{
					Id:   testFolder,
					Type: "resource-manager.folder",
				},
			},
		}).Times(1).Return(&servicecontrol.AuthorizeResponse{
		Subject: &servicecontrol.Subject{
			Type: &servicecontrol.Subject_UserAccount_{
				UserAccount: &servicecontrol.Subject_UserAccount{
					Id: testUser,
				},
			},
		},
	}, nil)

	var md = grpc_metadata.Pairs(
		authorizationHeader, addPrefix(testTokenOK),
	)
	ctxReq := grpc_metadata.NewIncomingContext(ctx, md)
	ctxReq = xrequestid.WithRequestID(ctxReq, testReqID)
	userID, err := c.Authorize(ctxReq, testPermission)
	at.NoError(err)
	at.Equal("user-"+testUser, userID)

	// Error
	asMock.On(
		"Authorize",
		mock.MatchedBy(asmock.ContainsGRPCMetadataStrings(t,
			requestIDHeader, testReqID,
		)),
		&servicecontrol.AuthorizeRequest{
			Identity: &servicecontrol.AuthorizeRequest_IamToken{
				IamToken: testTokenBad,
			},
			Permission: testPermission,
			ResourcePath: []*servicecontrol.Resource{
				&servicecontrol.Resource{
					Id:   testFolder,
					Type: "resource-manager.folder",
				},
			},
		}).Times(1).Return(nil, testErr)

	md = grpc_metadata.Pairs(
		authorizationHeader, addPrefix(testTokenBad),
	)
	ctxReq = grpc_metadata.NewIncomingContext(ctx, md)
	ctxReq = xrequestid.WithRequestID(ctxReq, testReqID)
	userID, err = c.Authorize(ctxReq, testPermission)
	at.Equal("", userID)
	at.Error(err)
	at.True(strings.Contains(err.Error(), testErr.Error()))

	// Bad prefix
	asMock.On(
		"Authorize",
		mock.MatchedBy(asmock.ContainsGRPCMetadataStrings(t,
			requestIDHeader, testReqID,
		)),
		&servicecontrol.AuthorizeRequest{
			Identity: &servicecontrol.AuthorizeRequest_IamToken{
				IamToken: testTokenBadPrefix,
			},
			Permission: testPermission,
			ResourcePath: []*servicecontrol.Resource{
				&servicecontrol.Resource{
					Id:   testFolder,
					Type: "resource-manager.folder",
				},
			},
		}).Times(1).Return(nil, testErr)

	md = grpc_metadata.Pairs(
		authorizationHeader, addPrefix(testTokenBadPrefix),
	)
	ctxReq = grpc_metadata.NewIncomingContext(ctx, md)
	ctxReq = xrequestid.WithRequestID(ctxReq, testReqID)
	userID, err = c.Authorize(ctxReq, testPermission)
	at.Equal("", userID)
	at.Error(err)
	at.True(strings.Contains(err.Error(), testErr.Error()))
}
