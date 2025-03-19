package cloudauth

import (
	"testing"
	"time"

	"golang.org/x/net/context"
	grpc "google.golang.org/grpc"

	pb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1"
)

func TestAuthenticateToken(t *testing.T) {
	cred := NewIAMToken("token")
	checkAuthenticateResponse(cred, t)
}

func TestAuthenticateSignatureV2(t *testing.T) {
	cred := NewAccessKeySignatureV2("keyId", "s2s", "aaabbbcccdddeeefff", HmacSha256)
	checkAuthenticateResponse(cred, t)
}

func TestAuthenticateSignatureV4(t *testing.T) {
	cred := NewAccessKeySignatureV4("keyId", "s2s", "0000000000", "service", "region", time.Now())
	checkAuthenticateResponse(cred, t)
}

func TestAuthenticateApiKey(t *testing.T) {
	cred := NewAPIKey("api-key")
	checkAuthenticateResponse(cred, t)
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

func (c *mockAccessServiceClient) BulkAuthorize(ctx context.Context, in *pb.BulkAuthorizeRequest, opts ...grpc.CallOption) (*pb.BulkAuthorizeResponse, error) {
	return &pb.BulkAuthorizeResponse{
		Subject: &pb.Subject{
			Type: &pb.Subject_ServiceAccount_{ServiceAccount: &pb.Subject_ServiceAccount{
				Id:       "saId",
				FolderId: "folderId",
			}},
		},
	}, nil
}

func checkAuthenticateResponse(a Authenticatable, t *testing.T) {
	client := &AccessServiceClient{&mockAccessServiceClient{}}
	subj, err := client.Authenticate(context.Background(), a)
	if err != nil {
		t.Error(err)
	}
	ua := subj.(UserAccount)
	if ua.ID != "userId" {
		t.Errorf("Invalid id %s", ua.ID)
	}
}

func TestAuthorizeSubject(t *testing.T) {
	cred := ServiceAccount{"saId", "folderId"}
	checkAuthorizeResponse(cred, t)
}

func TestAuthorizeToken(t *testing.T) {
	cred := IAMToken("token")
	checkAuthorizeResponse(cred, t)
}

func TestAuthorizeSignatureV2(t *testing.T) {
	cred := NewAccessKeySignatureV2("keyId", "s2s", "aaabbbcccdddeeefff", HmacSha256)
	checkAuthorizeResponse(cred, t)
}

func TestAuthorizeSignatureV4(t *testing.T) {
	cred := NewAccessKeySignatureV4("keyId", "s2s", "0000000000", "service", "region", time.Now())
	checkAuthorizeResponse(cred, t)
}

func TestAuthorizeApiKey(t *testing.T) {
	cred := APIKey("api-key")
	checkAuthorizeResponse(cred, t)
}

func TestAuthenticateAndAuthorize(t *testing.T) {
	client := &AccessServiceClient{&mockAccessServiceClient{}}

	cred := NewAccessKeySignatureV2("keyId", "s2s", "aaabbbcccdddeeefff", HmacSha256)
	subj, err := client.Authenticate(context.Background(), cred)
	if err != nil {
		t.Error(err)
	}

	checkAuthorizeResponse(subj.(Authorizable), t)
}

func checkAuthorizeResponse(a Authorizable, t *testing.T) {
	client := &AccessServiceClient{&mockAccessServiceClient{}}
	subj, err := client.Authorize(context.Background(), a, "compute.publicTemplates.list", Resource{"id", "type"})
	if err != nil {
		t.Error(err)
	}
	sa := subj.(ServiceAccount)
	if sa.ID != "saId" {
		t.Errorf("Invalid id %s", sa.ID)
	}
}
