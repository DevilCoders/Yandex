package asmock

import (
	"context"
	"testing"

	"google.golang.org/grpc/metadata"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"

	"google.golang.org/grpc"

	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1"
)

func TestIamMockServer(t *testing.T) {
	at := assert.New(t)

	asMock, address, asMockCancel, err := NewMockAccessService(nil)
	if err != nil {
		t.Fatalf("Can't start access service mock: %v", err)
	}
	defer asMockCancel()

	metaName := "test-meta-name"
	metaVal := "testMetaVal"
	testToken := "testToken"
	testPermission := "testPermission"
	testFolder := "testFolderID"
	testUser := "test-user"

	asMock.On(
		"Authorize",
		mock.MatchedBy(ContainsGRPCMetadataStrings(t,
			metaName, metaVal,
		)),
		&servicecontrol.AuthorizeRequest{
			Identity: &servicecontrol.AuthorizeRequest_IamToken{
				IamToken: testToken,
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

	ctx := context.Background()
	conn, err := grpc.DialContext(
		ctx,
		address,
		grpc.WithInsecure(),
	)
	at.NoError(err)

	client := cloudauth.NewAccessServiceClient(conn)
	ctxReq := metadata.AppendToOutgoingContext(ctx, metaName, metaVal)
	subj, err := client.Authorize(ctxReq, cloudauth.IAMToken(testToken), testPermission, cloudauth.ResourceFolder(testFolder))
	at.NoError(err)
	at.NotNil(subj)
}
