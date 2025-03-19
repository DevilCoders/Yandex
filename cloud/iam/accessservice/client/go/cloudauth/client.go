/* This package provides a wrapper over Access Service gRPC client.

Create a new client with recommended settings like this:

	import (
		grpc_retry "github.com/grpc-ecosystem/go-grpc-middleware/retry"
		"google.golang.org/grpc"
		"google.golang.org/grpc/credentials"
		"google.golang.org/grpc/keepalive"
		"google.golang.org/grpc/metadata"
	)

	type contextKey string

	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()
	retryInterceptor := grpc_retry.UnaryClientInterceptor(
		grpc_retry.WithMax(7),
		grpc_retry.WithPerRetryTimeout(2*time.Second),
	)
	var requestIdInterceptor grpc.UnaryClientInterceptor = func(ctx context.Context, method string, req, reply interface{},
		cc *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {

		reqId := ctx.Value(contextKey("requestId"))
		if reqId != nil {
			grpc.SetHeader(ctx, metadata.Pairs(cloudauth.RequestID, reqId.(string)))
		}
		return retryInterceptor(ctx, method, req, reply, cc, invoker, opts...)
	}
	tls, err := credentials.NewClientTLSFromFile("/etc/ssl/certs/ca-certificates.crt", "")
	if err != nil {
		panic(err)
	}
	conn, err := grpc.DialContext(ctx, "as.private-api.cloud-preprod.yandex.net:4286", // prod: as.private-api.cloud.yandex.net:4286
		grpc.WithTransportCredentials(tls),
		grpc.WithUnaryInterceptor(requestIdInterceptor),
		grpc.WithUserAgent("MY-APP"),
		grpc.WithKeepaliveParams(keepalive.ClientParameters{
				Time: 10 * time.Second,
				Timeout: 1 * time.Second,
				PermitWithoutStream: true,
    }),

	)
	if err != nil {
		panic(err)
	}
	client := cloudauth.NewAccessServiceClient(conn)

For authentication, create credentials using one of NewIAMToken(), NewAccessKeySignatureV2(),
NewAccessKeySignatureV4(), NewAPIKey() and pass them to client.Authenticate():

	reqCtx := context.WithValue(context.Background(), contextKey("requestId"), "PUT-REAL-REQUEST-ID-HERE")
	subject, err := client.Authenticate(reqCtx, cred)

Check the returned subject like this:

	switch s := subject.(type) {
	case cloudauth.AnonymousAccount:
		fmt.Println("Anonymous")
	case cloudauth.UserAccount:
		fmt.Println(s.ID)
	case cloudauth.ServiceAccount:
		fmt.Println(s.ID, s.FolderID)
	}

To authorize a subject:
	_, err := client.Authorize(reqCtx, subject.(cloudauth.Authorizable), "permission",
		cloudauth.ResourceCloud("cloudId"), cloudauth.ResourceFolder("folderId"), ...)

Or, use credentials instead of a subject to authenticate and authorize in one call.
*/

package cloudauth

import (
	"context"
	"errors"
	"fmt"
	"time"

	"github.com/golang/protobuf/ptypes/timestamp"
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1"
)

const HmacSha1 = servicecontrol.AccessKeySignature_Version2Parameters_HMAC_SHA1
const HmacSha256 = servicecontrol.AccessKeySignature_Version2Parameters_HMAC_SHA256
const RequestID = "X-Request-Id"

const ResourceTypeCloud = "resource-manager.cloud"
const ResourceTypeFolder = "resource-manager.folder"
const ResourceTypeServiceAccount = "iam.serviceAccount"

type ContextKey string

type AccessServiceClient struct {
	client servicecontrol.AccessServiceClient
}

type Authenticatable interface {
	toAuthenticateRequest() *servicecontrol.AuthenticateRequest
}

type Authorizable interface {
	toAuthorizeRequest(permission string, resourcePath ...Resource) *servicecontrol.AuthorizeRequest
}

type Resource struct {
	ID   string
	Type string
}

func ResourceCloud(id string) Resource {
	return Resource{id, ResourceTypeCloud}
}

func ResourceFolder(id string) Resource {
	return Resource{id, ResourceTypeFolder}
}

func ResourceServiceAccount(id string) Resource {
	return Resource{id, ResourceTypeServiceAccount}
}

type Subject interface {
	isSubject()
}

type AnonymousAccount struct{}

type UserAccount struct {
	ID string
}

type ServiceAccount struct {
	ID       string
	FolderID string
}

func (AnonymousAccount) isSubject() {}
func (UserAccount) isSubject()      {}
func (ServiceAccount) isSubject()   {}

func (AnonymousAccount) String() string {
	return "<AnonymousAccount>"
}

func (ua UserAccount) String() string {
	return fmt.Sprintf("<UserAccount: id %s>", ua.ID)
}

func (sa ServiceAccount) String() string {
	return fmt.Sprintf("<ServiceAccount: id %s, folder_id %s>", sa.ID, sa.FolderID)
}

func (AnonymousAccount) toAuthorizeRequest(permission string, resourcePath ...Resource) *servicecontrol.AuthorizeRequest {
	req := makeAuthorizeRequest(permission, resourcePath...)
	req.Identity = &servicecontrol.AuthorizeRequest_Subject{Subject: &servicecontrol.Subject{
		Type: &servicecontrol.Subject_AnonymousAccount_{AnonymousAccount: &servicecontrol.Subject_AnonymousAccount{}},
	}}
	return req
}

func (ua UserAccount) toAuthorizeRequest(permission string, resourcePath ...Resource) *servicecontrol.AuthorizeRequest {
	req := makeAuthorizeRequest(permission, resourcePath...)
	req.Identity = &servicecontrol.AuthorizeRequest_Subject{Subject: &servicecontrol.Subject{
		Type: &servicecontrol.Subject_UserAccount_{UserAccount: &servicecontrol.Subject_UserAccount{
			Id: ua.ID,
		}},
	}}
	return req
}

func (sa ServiceAccount) toAuthorizeRequest(permission string, resourcePath ...Resource) *servicecontrol.AuthorizeRequest {
	req := makeAuthorizeRequest(permission, resourcePath...)
	req.Identity = &servicecontrol.AuthorizeRequest_Subject{Subject: &servicecontrol.Subject{
		Type: &servicecontrol.Subject_ServiceAccount_{ServiceAccount: &servicecontrol.Subject_ServiceAccount{
			Id:       sa.ID,
			FolderId: sa.FolderID,
		}},
	}}
	return req
}

type IAMToken string

func (t IAMToken) toAuthenticateRequest() *servicecontrol.AuthenticateRequest {
	return &servicecontrol.AuthenticateRequest{
		Credentials: &servicecontrol.AuthenticateRequest_IamToken{IamToken: string(t)},
	}
}

func (t IAMToken) toAuthorizeRequest(permission string, resourcePath ...Resource) *servicecontrol.AuthorizeRequest {
	req := makeAuthorizeRequest(permission, resourcePath...)
	req.Identity = &servicecontrol.AuthorizeRequest_IamToken{IamToken: string(t)}
	return req
}

type accessKeySignature struct {
	KeyID        string
	StringToSign string
	Signature    string
}

func (a accessKeySignature) toAccessKeySignature() *servicecontrol.AccessKeySignature {
	return &servicecontrol.AccessKeySignature{
		AccessKeyId:  a.KeyID,
		StringToSign: a.StringToSign,
		Signature:    a.Signature,
	}
}

type AccessKeySignatureV2 struct {
	accessKeySignature
	SignatureMethod servicecontrol.AccessKeySignature_Version2Parameters_SignatureMethod
}

func (a AccessKeySignatureV2) toAccessKeySignature() *servicecontrol.AccessKeySignature {
	aks := a.accessKeySignature.toAccessKeySignature()
	aks.Parameters = &servicecontrol.AccessKeySignature_V2Parameters{
		V2Parameters: &servicecontrol.AccessKeySignature_Version2Parameters{
			SignatureMethod: a.SignatureMethod,
		},
	}
	return aks
}

func (a AccessKeySignatureV2) toAuthenticateRequest() *servicecontrol.AuthenticateRequest {
	return &servicecontrol.AuthenticateRequest{
		Credentials: &servicecontrol.AuthenticateRequest_Signature{Signature: a.toAccessKeySignature()},
	}
}

func (a AccessKeySignatureV2) toAuthorizeRequest(permission string, resourcePath ...Resource) *servicecontrol.AuthorizeRequest {
	req := makeAuthorizeRequest(permission, resourcePath...)
	req.Identity = &servicecontrol.AuthorizeRequest_Signature{Signature: a.toAccessKeySignature()}
	return req
}

type AccessKeySignatureV4 struct {
	accessKeySignature
	SignedAt time.Time
	Service  string
	Region   string
}

func (a AccessKeySignatureV4) toAccessKeySignature() *servicecontrol.AccessKeySignature {
	aks := a.accessKeySignature.toAccessKeySignature()
	aks.Parameters = &servicecontrol.AccessKeySignature_V4Parameters{
		V4Parameters: &servicecontrol.AccessKeySignature_Version4Parameters{
			SignedAt: &timestamp.Timestamp{Seconds: a.SignedAt.Unix()},
			Service:  a.Service,
			Region:   a.Region,
		},
	}
	return aks
}

func (a AccessKeySignatureV4) toAuthenticateRequest() *servicecontrol.AuthenticateRequest {
	return &servicecontrol.AuthenticateRequest{
		Credentials: &servicecontrol.AuthenticateRequest_Signature{Signature: a.toAccessKeySignature()},
	}
}

func (a AccessKeySignatureV4) toAuthorizeRequest(permission string, resourcePath ...Resource) *servicecontrol.AuthorizeRequest {
	req := makeAuthorizeRequest(permission, resourcePath...)
	req.Identity = &servicecontrol.AuthorizeRequest_Signature{Signature: a.toAccessKeySignature()}
	return req
}

type APIKey string

func (k APIKey) toAuthenticateRequest() *servicecontrol.AuthenticateRequest {
	return &servicecontrol.AuthenticateRequest{
		Credentials: &servicecontrol.AuthenticateRequest_ApiKey{ApiKey: string(k)},
	}
}

func (k APIKey) toAuthorizeRequest(permission string, resourcePath ...Resource) *servicecontrol.AuthorizeRequest {
	req := makeAuthorizeRequest(permission, resourcePath...)
	req.Identity = &servicecontrol.AuthorizeRequest_ApiKey{ApiKey: string(k)}
	return req
}

func makeAccessKeySignature(keyID, stringToSign, signature string) accessKeySignature {
	return accessKeySignature{
		KeyID:        keyID,
		StringToSign: stringToSign,
		Signature:    signature,
	}
}

func NewAccessKeySignatureV2(keyID, stringToSign, signature string,
	method servicecontrol.AccessKeySignature_Version2Parameters_SignatureMethod) *AccessKeySignatureV2 {
	return &AccessKeySignatureV2{
		accessKeySignature: makeAccessKeySignature(keyID, stringToSign, signature),
		SignatureMethod:    method,
	}
}

func NewAccessKeySignatureV4(keyID, stringToSign, signature, service, region string, signedAt time.Time) *AccessKeySignatureV4 {
	return &AccessKeySignatureV4{
		accessKeySignature: makeAccessKeySignature(keyID, stringToSign, signature),
		SignedAt:           signedAt,
		Service:            service,
		Region:             region,
	}
}

func NewIAMToken(token string) IAMToken {
	return IAMToken(token)
}

func NewAPIKey(key string) APIKey {
	return APIKey(key)
}

func NewAccessServiceClient(cc *grpc.ClientConn) *AccessServiceClient {
	return &AccessServiceClient{servicecontrol.NewAccessServiceClient(cc)}
}

func (acc *AccessServiceClient) Authenticate(ctx context.Context, a Authenticatable) (Subject, error) {
	res, err := acc.client.Authenticate(ctx, a.toAuthenticateRequest())
	if err != nil {
		return nil, err
	}
	return subjectFromGrpc(res.Subject), nil
}

func (acc *AccessServiceClient) Authorize(ctx context.Context, a Authorizable, permission string, resourcePath ...Resource) (Subject, error) {
	if len(resourcePath) == 0 {
		return nil, errors.New("At least one resource is required")
	}
	res, err := acc.client.Authorize(ctx, a.toAuthorizeRequest(permission, resourcePath...))
	if err != nil {
		return nil, err
	}
	return subjectFromGrpc(res.Subject), nil
}

func subjectFromGrpc(subject *servicecontrol.Subject) Subject {
	switch subjectType := subject.Type.(type) {
	case *servicecontrol.Subject_AnonymousAccount_:
		return AnonymousAccount{}
	case *servicecontrol.Subject_UserAccount_:
		return UserAccount{subjectType.UserAccount.Id}
	case *servicecontrol.Subject_ServiceAccount_:
		return ServiceAccount{subjectType.ServiceAccount.Id, subjectType.ServiceAccount.FolderId}
	}
	return nil
}

func makeAuthorizeRequest(permission string, resourcePath ...Resource) *servicecontrol.AuthorizeRequest {
	grpcResources := make([]*servicecontrol.Resource, len(resourcePath))
	for i, res := range resourcePath {
		grpcResources[i] = &servicecontrol.Resource{
			Id:   res.ID,
			Type: res.Type,
		}
	}
	return &servicecontrol.AuthorizeRequest{
		Permission:   permission,
		ResourcePath: grpcResources,
	}
}
