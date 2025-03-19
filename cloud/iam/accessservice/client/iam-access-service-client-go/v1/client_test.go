package cloudauth

import (
	"reflect"
	"strings"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"go.uber.org/zap"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/metadata"

	pb "a.yandex-team.ru/cloud/iam/accessservice/client/iam-access-service-api-proto/v1/go"
)

func TestGetSubjectID(t *testing.T) {
	assert.Equal(t, "", GetSubjectID(nil))
	assert.Equal(t, "", GetSubjectID(&AnonymousAccount{}))
	assert.Equal(t, "user", GetSubjectID(&UserAccount{ID: "user"}))
	assert.Equal(t, "service", GetSubjectID(&ServiceAccount{ID: "service"}))
}

func TestAuthenticateToken(t *testing.T) {
	cred := NewIAMToken("token")
	checkAuthenticateResponse(t, cred)
}

func TestAuthenticateCookie(t *testing.T) {
	cred := NewIAMCookie("cookie")
	checkAuthenticateResponse(t, cred)
}

func TestAuthenticateSignatureV2(t *testing.T) {
	cred := NewAccessKeySignatureV2("keyId", "s2s", "aaabbbcccdddeeefff", HmacSha256)
	checkAuthenticateResponse(t, cred)
}

func TestAuthenticateSignatureV4(t *testing.T) {
	cred := NewAccessKeySignatureV4("keyId", "s2s", "0000000000", "service", "region", time.Now())
	checkAuthenticateResponse(t, cred)
}

func TestAuthenticateApiKey(t *testing.T) {
	cred := NewAPIKey("api-key")
	checkAuthenticateResponse(t, cred)
}

type mockAccessServiceClient struct{}

func (c *mockAccessServiceClient) Authenticate(ctx context.Context, in *pb.AuthenticateRequest, opts ...grpc.CallOption) (*pb.AuthenticateResponse, error) {
	return &pb.AuthenticateResponse{
		Subject: &pb.Subject{
			Type: &pb.Subject_UserAccount_{UserAccount: &pb.Subject_UserAccount{
				Id: "userId",
			}},
		},
	}, nil
}

func (c *mockAccessServiceClient) Authorize(ctx context.Context, in *pb.AuthorizeRequest, opts ...grpc.CallOption) (*pb.AuthorizeResponse, error) {
	return &pb.AuthorizeResponse{
		Subject: &pb.Subject{
			Type: &pb.Subject_ServiceAccount_{ServiceAccount: &pb.Subject_ServiceAccount{
				Id:       "saId",
				FolderId: "folderId",
			}},
		},
	}, nil
}

// This mock method returns errors for actions where isPermissionDenied() returns true.
// It doesn't handle ResultFilter or ResultField options. See the more realistic integration tests at the end.
func (c *mockAccessServiceClient) BulkAuthorize(ctx context.Context, in *pb.BulkAuthorizeRequest, opts ...grpc.CallOption) (*pb.BulkAuthorizeResponse, error) {

	errors := make([]*pb.BulkAuthorizeResponse_Result, 0)

	// Actions
	for _, action := range in.GetActions().GetItems() {
		if isPermissionDenied(action.Permission, action.ResourcePath) {
			errors = append(errors, buildGrpcResult(action.Permission, action.ResourcePath))
		}
	}

	// ActionMatrix
	if in.GetActionMatrix() != nil {
		for _, permission := range in.GetActionMatrix().Permissions {
			for _, resourcePath := range in.GetActionMatrix().ResourcePaths {
				if isPermissionDenied(permission, resourcePath.Path) {
					errors = append(errors, buildGrpcResult(permission, resourcePath.Path))
				}
			}
		}
	}

	return &pb.BulkAuthorizeResponse{
		Subject: in.GetSubject(),
		Results: &pb.BulkAuthorizeResponse_Results{
			Items: errors,
		},
	}, nil
}

// Looks for fake IDs with "bad_" prefix
func isPermissionDenied(permission string, resourcePath []*pb.Resource) bool {
	return strings.HasPrefix(permission, "bad_") || strings.HasPrefix(resourcePath[0].Id, "bad_")
}

func buildGrpcResult(permission string, resourcePath []*pb.Resource) *pb.BulkAuthorizeResponse_Result {
	return &pb.BulkAuthorizeResponse_Result{
		Permission:            permission,
		ResourcePath:          resourcePath,
		PermissionDeniedError: &pb.BulkAuthorizeResponse_Error{Message: "Permission denied"},
	}
}

func TestAuthorizeSubject(t *testing.T) {
	cred := ServiceAccount{"saId", "folderId"}
	checkAuthorizeResponse(t, cred)
}

func TestAuthorizeToken(t *testing.T) {
	cred := NewIAMToken("token")
	checkAuthorizeResponse(t, cred)
}

func TestAuthorizeSignatureV2(t *testing.T) {
	cred := NewAccessKeySignatureV2("keyId", "s2s", "aaabbbcccdddeeefff", HmacSha256)
	checkAuthorizeResponse(t, cred)
}

func TestAuthorizeSignatureV4(t *testing.T) {
	cred := NewAccessKeySignatureV4("keyId", "s2s", "0000000000", "service", "region", time.Now())
	checkAuthorizeResponse(t, cred)
}

func TestAuthorizeApiKey(t *testing.T) {
	cred := NewAPIKey("api-key")
	checkAuthorizeResponse(t, cred)
}

func TestAuthenticateAndAuthorize(t *testing.T) {
	client := &DefaultAccessServiceClient{&mockAccessServiceClient{}}

	cred := NewAccessKeySignatureV2("keyId", "s2s", "aaabbbcccdddeeefff", HmacSha256)
	subj, err := client.Authenticate(context.Background(), cred)
	if err != nil {
		t.Error(err)
	}

	checkAuthorizeResponse(t, subj.(Authorizable))
}

// source version with cached client
/*
func createClients(t *testing.T) (*DefaultAccessServiceClient, *AccessServiceCachedClient) {
	t.Helper()

	client := &DefaultAccessServiceClient{&mockAccessServiceClient{}}

	cachedClient, err := NewAccessServiceCachedClient(
		client,
		&cache.DefaultTracer{},
		AccessServiceCachedClientConfig{
			Lifetime:       time.Second,
			UpdateInterval: 500 * time.Millisecond,
			IAMTimeout:     100 * time.Millisecond,
			CacheTimeout:   250 * time.Millisecond,
		},
	)
	if err != nil {
		t.Fatalf("Error on create AccessServiceCachedClient: %v", err)
	}

	return client, cachedClient
}
*/

func createClients(t *testing.T) *DefaultAccessServiceClient {
	t.Helper()

	client := &DefaultAccessServiceClient{&mockAccessServiceClient{}}

	return client
}

func checkAuthenticateResponse(t *testing.T, a Authenticatable) {
	t.Helper()

	//client, cachedClient := createClients(t)
	client := createClients(t)
	checkAuthenticateClientResponse(t, client, a)
	//checkAuthenticateClientResponse(t, cachedClient, a)
}

func checkAuthenticateClientResponse(t *testing.T, client AccessServiceClient, a Authenticatable) {
	t.Helper()

	subj, err := client.Authenticate(context.Background(), a)
	if err != nil {
		t.Error(err)
	}
	ua := subj.(UserAccount)
	if ua.ID != "userId" {
		t.Errorf("Invalid id %s", ua.ID)
	}
}

func checkAuthorizeResponse(t *testing.T, a Authorizable) {
	t.Helper()

	//client, cachedClient := createClients(t)
	client := createClients(t)
	checkAuthorizeClientResponse(t, client, a)
	//checkAuthorizeClientResponse(t, cachedClient, a)
}

func checkAuthorizeClientResponse(t *testing.T, client AccessServiceClient, a Authorizable) {
	t.Helper()

	//subj, err := client.Authorize(context.Background(), a, permissions.ComputeAddressesList, Resource{"id", "type"})
	subj, err := client.Authorize(context.Background(), a, "compute.addresses.list", Resource{"id", "type"})
	if err != nil {
		t.Error(err)
	}
	sa := subj.(ServiceAccount)
	if sa.ID != "saId" {
		t.Errorf("Invalid id %s", sa.ID)
	}
}

func TestBulkAuthorizeActions(t *testing.T) {
	client := &DefaultAccessServiceClient{&mockAccessServiceClient{}}
	req := &BulkAuthorizeRequest{
		Authorizable: ServiceAccount{"saId", "folderId"},
		Actions: []*Action{
			{Permission: "perm1", ResourcePath: []*Resource{{"id1", "type1"}}},
			{Permission: "bad_perm2", ResourcePath: []*Resource{{"id2", "type2"}}},
			{Permission: "perm3", ResourcePath: []*Resource{{"id3", "type3"}}},
			{Permission: "perm4", ResourcePath: []*Resource{{"bad_id4", "type4"}}},
			{Permission: "perm5", ResourcePath: []*Resource{{"id5", "type5"}}},
		},
	}
	resp, err := client.BulkAuthorize(context.Background(), req)
	if err != nil {
		t.Error(err)
	}
	expected := &BulkAuthorizeResponse{
		Subject: ServiceAccount{ID: "saId", FolderID: "folderId"},
		Results: []*BulkAuthorizeResult{
			buildResult("bad_perm2", []*Resource{{ID: "id2", Type: "type2"}}),
			buildResult("perm4", []*Resource{{ID: "bad_id4", Type: "type4"}}),
		},
	}
	if !reflect.DeepEqual(resp, expected) {
		t.Errorf("Expected %v", expected)
		t.Errorf("Found %v", resp)
	}
}

func TestBulkAuthorizeActionMatrix(t *testing.T) {
	client := &DefaultAccessServiceClient{&mockAccessServiceClient{}}
	req := &BulkAuthorizeRequest{
		Authorizable: ServiceAccount{"saId", "folderId"},
		ActionMatrix: &ActionMatrix{
			Permissions: []string{"perm1", "bad_perm2"},
			ResourcePaths: [][]*Resource{
				{{"bad_id1", "type1"}},
				{{"id2", "type2"}},
			},
		},
	}
	resp, err := client.BulkAuthorize(context.Background(), req)
	if err != nil {
		t.Error(err)
	}
	expected := &BulkAuthorizeResponse{
		Subject: ServiceAccount{ID: "saId", FolderID: "folderId"},
		Results: []*BulkAuthorizeResult{
			buildResult("perm1", []*Resource{{ID: "bad_id1", Type: "type1"}}),
			buildResult("bad_perm2", []*Resource{{ID: "bad_id1", Type: "type1"}}),
			buildResult("bad_perm2", []*Resource{{ID: "id2", Type: "type2"}}),
		},
	}
	if !reflect.DeepEqual(resp, expected) {
		t.Errorf("Expected %v", expected)
		t.Errorf("Found %v", resp)
	}
}

func buildResult(permission string, resourcePath []*Resource) *BulkAuthorizeResult {
	return &BulkAuthorizeResult{
		Permission:            permission,
		ResourcePath:          resourcePath,
		PermissionDeniedError: "Permission denied",
	}
}

// To run the following integration tests:
// - Point `accessServiceEndpoint` to a real server
// - Remove the first "x" in test names
// - Run `go test` in this folder.
// This test expect YDB to be populated with db_populate (identity project).
// A note about making changes in code. After the changes, from `cloud-go` project root:
// - Run `make gomod.vendor && make generate && make format`
// - Run `make check-affected test-affected` to see lint messages.

var accessServiceEndpoint = "localhost:4286"

var resourcePath1 = []*Resource{{ID: "bat0000service0cloud", Type: "resource-manager.cloud"}}
var resourcePath2 = []*Resource{{ID: "bat00000000000000002", Type: "resource-manager.folder"}}
var permissionGet = "iam.clouds.get"
var permissionUpdate = "iam.cloud.update"

var goodAction1 = &Action{Permission: permissionGet, ResourcePath: resourcePath1}
var badAction1 = &Action{Permission: permissionUpdate, ResourcePath: resourcePath1}
var goodAction2 = &Action{Permission: permissionGet, ResourcePath: resourcePath2}
var badAction2 = &Action{Permission: permissionUpdate, ResourcePath: resourcePath2}

func xTestIntegrationBulkAuthorizeActionsAllFailed(t *testing.T) {
	client, ctx, cancel := newClientForRequest(t, "test bulk-authorize actions all-failed")
	defer cancel()

	account := UserAccount{ID: "d2670000000000000009"}

	request := BulkAuthorizeRequest{
		Authorizable: account,
		ResultFilter: AllFailed,
		Actions:      []*Action{goodAction1, badAction1, goodAction2, badAction2},
	}

	expected := &BulkAuthorizeResponse{
		Subject: account,
		Results: []*BulkAuthorizeResult{
			buildResult(badAction1.Permission, badAction1.ResourcePath),
			buildResult(badAction2.Permission, badAction2.ResourcePath),
		},
	}

	verifyBulkAuthorizeRequest(ctx, request, expected, client, t)
}

func xTestIntegrationBulkAuthorizeActionsFirstFailedFieldMask(t *testing.T) {
	client, ctx, cancel := newClientForRequest(t, "test bulk-authorize actions first-failed field-mask")
	defer cancel()

	account := UserAccount{ID: "d2670000000000000009"}

	request := BulkAuthorizeRequest{
		Authorizable: account,
		ResultFilter: FirstFailed,                                      // only first error will be expected
		FieldMask:    []ResultField{Permission, PermissionDeniedError}, // only include these fields in results
		Actions:      []*Action{goodAction1, badAction1, goodAction2, badAction2},
	}

	expected := &BulkAuthorizeResponse{
		Subject: account,
		Results: []*BulkAuthorizeResult{
			buildResult(badAction1.Permission, []*Resource{}), // default value for non-included fields
		},
	}

	verifyBulkAuthorizeRequest(ctx, request, expected, client, t)
}

func xTestIntegrationBulkAuthorizeActionMatrixAllFailed(t *testing.T) {
	client, ctx, cancel := newClientForRequest(t, "test bulk-authorize actions-matrix all-failed")
	defer cancel()

	account := UserAccount{ID: "d2670000000000000009"}

	request := BulkAuthorizeRequest{
		Authorizable: account,
		ResultFilter: AllFailed,
		ActionMatrix: &ActionMatrix{
			Permissions:   []string{permissionGet, permissionUpdate},
			ResourcePaths: [][]*Resource{resourcePath1, resourcePath2},
		},
	}

	expected := &BulkAuthorizeResponse{
		Subject: account,
		Results: []*BulkAuthorizeResult{
			buildResult(permissionUpdate, resourcePath1),
			buildResult(permissionUpdate, resourcePath2),
		},
	}

	verifyBulkAuthorizeRequest(ctx, request, expected, client, t)
}

func verifyBulkAuthorizeRequest(ctx context.Context, request BulkAuthorizeRequest, expected *BulkAuthorizeResponse, client *DefaultAccessServiceClient, t *testing.T) {
	response, err := client.BulkAuthorize(ctx, &request)

	if err != nil {
		t.Errorf("Error: %v\n", err)
	}

	if !reflect.DeepEqual(response, expected) {
		t.Errorf("Expected %v", expected)
		t.Errorf("Found %v", response)
	}
}

func newClientForRequest(t *testing.T, requestID string) (*DefaultAccessServiceClient, context.Context, context.CancelFunc) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	ctx = metadata.AppendToOutgoingContext(ctx, "X-Request-ID", requestID)
	client := newClient(ctx, accessServiceEndpoint, t)
	return client, ctx, cancel
}

func newClient(ctx context.Context, target string, t *testing.T) *DefaultAccessServiceClient {
	conn, err := grpc.DialContext(ctx, target,
		grpc.WithInsecure(),
		grpc.WithUserAgent("lib test"),
		grpc.WithWriteBufferSize(128*1024),
		grpc.WithReadBufferSize(128*1024))
	if err != nil {
		t.Errorf("Error: %v\n", zap.Any("err", err))
	}
	client := &DefaultAccessServiceClient{client: pb.NewAccessServiceClient(conn)}
	return client
}
