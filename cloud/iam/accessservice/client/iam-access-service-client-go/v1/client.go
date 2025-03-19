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
NewAccessKeySignatureV4(), NewAPIKey(), NewIAMCookie() and pass them to client.Authenticate():

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
	"strconv"
	"time"

	"github.com/golang/protobuf/ptypes/timestamp"
	"google.golang.org/grpc"
	"google.golang.org/protobuf/types/known/fieldmaskpb"

	pb "a.yandex-team.ru/cloud/iam/accessservice/client/iam-access-service-api-proto/v1/go"
)

const HmacSha1 = pb.AccessKeySignature_Version2Parameters_HMAC_SHA1
const HmacSha256 = pb.AccessKeySignature_Version2Parameters_HMAC_SHA256
const RequestID = "X-Request-Id"

const ResourceTypeCloud = "resource-manager.cloud"
const ResourceTypeFolder = "resource-manager.folder"
const ResourceTypeServiceAccount = "iam.serviceAccount"

type ContextKey string

//go:generate gmg --dst ./mocks/{}_gomock.go
type AccessServiceClient interface {
	Authenticate(ctx context.Context, a Authenticatable) (Subject, error)
	Authorize(ctx context.Context, a Authorizable, permission string, resourcePath ...Resource) (Subject, error)
}

var _ AccessServiceClient = &DefaultAccessServiceClient{}

type DefaultAccessServiceClient struct {
	client pb.AccessServiceClient
}

type Authenticatable interface {
	toAuthenticateRequest() *pb.AuthenticateRequest
}

type Authorizable interface {
	setAuthorizeIdentity(req *pb.AuthorizeRequest)
	setBulkAuthorizeIdentity(req *pb.BulkAuthorizeRequest)
	// hashingString returns a uniq string for object which uses for generate hash for IAM caching client.
	// It must use all fields from struct.
	hashingString() string
}

type Resource struct {
	ID   string
	Type string
}

func (r Resource) String() string {
	return fmt.Sprintf("{%v %v}", r.ID, r.Type)
}

// "bb.yandex-team.ru/cloud/cloud-go/iam/codegen/resources"
//var ResourceGizmo = Resource{"gizmo", resources.IamGizmo}
var ResourceGizmo = Resource{"gizmo", "iam.gizmo"}

func ResourceCloud(id string) Resource {
	return Resource{id, ResourceTypeCloud}
}

func ResourceFolder(id string) Resource {
	return Resource{id, ResourceTypeFolder}
}

func ResourceServiceAccount(id string) Resource {
	return Resource{id, ResourceTypeServiceAccount}
}

type BulkAuthorizeRequest struct {
	Authorizable Authorizable
	Actions      []*Action
	ActionMatrix *ActionMatrix
	ResultFilter ResultFilter
	FieldMask    []ResultField
}

type ResultField string

const (
	Permission            ResultField = "permission"
	ResourcePath          ResultField = "resource_path"
	PermissionDeniedError ResultField = "permission_denied_error"
)

type ResultFilter int32

const (
	AllFailed   = ResultFilter(pb.BulkAuthorizeRequest_ALL_FAILED)
	FirstFailed = ResultFilter(pb.BulkAuthorizeRequest_FIRST_FAILED)
)

type Action struct {
	Permission   string
	ResourcePath []*Resource
}

type ActionMatrix struct {
	Permissions   []string
	ResourcePaths [][]*Resource
}

type BulkAuthorizeResponse struct {
	Subject              Subject
	UnauthenticatedError string
	Results              []*BulkAuthorizeResult
}

type BulkAuthorizeResult struct {
	Permission            string
	ResourcePath          []*Resource
	PermissionDeniedError string
}

func (r BulkAuthorizeResult) String() string {
	return fmt.Sprintf("{%v %v %v}", r.Permission, r.ResourcePath, r.PermissionDeniedError)
}

type Subject interface {
	isSubject()
	subjectType() string
	subjectID() string
}

// GetSubjectID returns ID to be used for CreatedBy field in operations
func GetSubjectID(subj Subject) string {
	if subj == nil {
		return ""
	}
	return subj.subjectID()
}

// GetSubjectType returns string id of a subject type
func GetSubjectType(subj Subject) string {
	if subj == nil {
		return ""
	}
	return subj.subjectType()
}

var _ Authorizable = (*AnonymousAccount)(nil)

type AnonymousAccount struct{}

var _ Authorizable = (*UserAccount)(nil)

type UserAccount struct {
	ID string
}

var _ Authorizable = (*ServiceAccount)(nil)

type ServiceAccount struct {
	ID       string
	FolderID string
}

func (AnonymousAccount) isSubject() {}
func (UserAccount) isSubject()      {}
func (ServiceAccount) isSubject()   {}

func (a AnonymousAccount) subjectID() string {
	return ""
}

func (a AnonymousAccount) subjectType() string {
	return "AnonymousAccount"
}

func (a UserAccount) subjectID() string {
	return a.ID
}

func (a UserAccount) subjectType() string {
	return "UserAccount"
}

func (a ServiceAccount) subjectID() string {
	return a.ID
}

func (a ServiceAccount) subjectType() string {
	return "ServiceAccount"
}

func (a AnonymousAccount) String() string {
	return "<AnonymousAccount>"
}

func (a UserAccount) String() string {
	return fmt.Sprintf("<UserAccount: id %s>", a.ID)
}

func (a ServiceAccount) String() string {
	return fmt.Sprintf("<ServiceAccount: id %s, folder_id %s>", a.ID, a.FolderID)
}

func (a AnonymousAccount) hashingString() string {
	// hashingString must use all fields from Authorizable implementation. If someone add new field or change
	// something to a, compilation error will be raised.
	var _ struct{} = a
	return "<anonymous>"
}

func (a UserAccount) hashingString() string {
	// hashingString must use all fields from Authorizable implementation. If someone add new field or change
	// something to a, compilation error will be raised.
	var _ struct {
		ID string
	} = a
	return a.ID
}

func (a ServiceAccount) hashingString() string {
	// hashingString must use all fields from Authorizable implementation. If someone add new field or change
	// something to a, compilation error will be raised.
	var _ struct {
		ID       string
		FolderID string
	} = a
	return "<" + a.ID + "><" + a.FolderID + ">"
}

func (a AnonymousAccount) setAuthorizeIdentity(req *pb.AuthorizeRequest) {
	req.Identity = &pb.AuthorizeRequest_Subject{Subject: &pb.Subject{
		Type: &pb.Subject_AnonymousAccount_{AnonymousAccount: &pb.Subject_AnonymousAccount{}},
	}}
}

func (a AnonymousAccount) setBulkAuthorizeIdentity(req *pb.BulkAuthorizeRequest) {
	req.Identity = &pb.BulkAuthorizeRequest_Subject{Subject: &pb.Subject{
		Type: &pb.Subject_AnonymousAccount_{AnonymousAccount: &pb.Subject_AnonymousAccount{}},
	}}
}

func (a UserAccount) setAuthorizeIdentity(req *pb.AuthorizeRequest) {
	req.Identity = &pb.AuthorizeRequest_Subject{Subject: &pb.Subject{
		Type: &pb.Subject_UserAccount_{UserAccount: &pb.Subject_UserAccount{
			Id: a.ID,
		}},
	}}
}

func (a UserAccount) setBulkAuthorizeIdentity(req *pb.BulkAuthorizeRequest) {
	req.Identity = &pb.BulkAuthorizeRequest_Subject{Subject: &pb.Subject{
		Type: &pb.Subject_UserAccount_{UserAccount: &pb.Subject_UserAccount{
			Id: a.ID,
		}},
	}}
}

func (a ServiceAccount) setAuthorizeIdentity(req *pb.AuthorizeRequest) {
	req.Identity = &pb.AuthorizeRequest_Subject{Subject: &pb.Subject{
		Type: &pb.Subject_ServiceAccount_{ServiceAccount: &pb.Subject_ServiceAccount{
			Id:       a.ID,
			FolderId: a.FolderID,
		}},
	}}
}

func (a ServiceAccount) setBulkAuthorizeIdentity(req *pb.BulkAuthorizeRequest) {
	req.Identity = &pb.BulkAuthorizeRequest_Subject{Subject: &pb.Subject{
		Type: &pb.Subject_ServiceAccount_{ServiceAccount: &pb.Subject_ServiceAccount{
			Id:       a.ID,
			FolderId: a.FolderID,
		}},
	}}
}

var _ Authorizable = (*IAMToken)(nil)

type IAMToken struct {
	Token string
}

func (t IAMToken) hashingString() string {
	// hashingString must use all fields from Authorizable implementation. If someone add new field or change
	// something to t, compilation error will be raised.
	var _ struct {
		Token string
	} = t
	return t.Token
}

func (t IAMToken) toAuthenticateRequest() *pb.AuthenticateRequest {
	return &pb.AuthenticateRequest{
		Credentials: &pb.AuthenticateRequest_IamToken{IamToken: t.Token},
	}
}

func (t IAMToken) setAuthorizeIdentity(req *pb.AuthorizeRequest) {
	req.Identity = &pb.AuthorizeRequest_IamToken{IamToken: t.Token}
}

func (t IAMToken) setBulkAuthorizeIdentity(req *pb.BulkAuthorizeRequest) {
	req.Identity = &pb.BulkAuthorizeRequest_IamToken{IamToken: t.Token}
}

type accessKeySignature struct {
	KeyID        string
	StringToSign string
	Signature    string
}

func (a accessKeySignature) toAccessKeySignature() *pb.AccessKeySignature {
	return &pb.AccessKeySignature{
		AccessKeyId:  a.KeyID,
		StringToSign: a.StringToSign,
		Signature:    a.Signature,
	}
}

func (a accessKeySignature) hashingString() string {
	// hashingString must use all fields from Authorizable implementation. If someone add new field or change
	// something to a, compilation error will be raised.
	var _ struct {
		KeyID        string
		StringToSign string
		Signature    string
	} = a
	return "<" + a.KeyID + "><" + a.StringToSign + "><" + a.Signature + ">"
}

var _ Authorizable = (*AccessKeySignatureV2)(nil)

type AccessKeySignatureV2 struct {
	KeySign         accessKeySignature
	SignatureMethod pb.AccessKeySignature_Version2Parameters_SignatureMethod
}

func (a AccessKeySignatureV2) hashingString() string {
	// hashingString must use all fields from Authorizable implementation. If someone add new field or change
	// something to a, compilation error will be raised.
	var _ struct {
		KeySign         accessKeySignature
		SignatureMethod pb.AccessKeySignature_Version2Parameters_SignatureMethod
	} = a
	return a.KeySign.hashingString() + "<" + strconv.Itoa(int(a.SignatureMethod)) + ">"
}

func (a AccessKeySignatureV2) toAccessKeySignature() *pb.AccessKeySignature {
	aks := a.KeySign.toAccessKeySignature()
	aks.Parameters = &pb.AccessKeySignature_V2Parameters{
		V2Parameters: &pb.AccessKeySignature_Version2Parameters{
			SignatureMethod: a.SignatureMethod,
		},
	}
	return aks
}

func (a AccessKeySignatureV2) toAuthenticateRequest() *pb.AuthenticateRequest {
	return &pb.AuthenticateRequest{
		Credentials: &pb.AuthenticateRequest_Signature{Signature: a.toAccessKeySignature()},
	}
}

func (a AccessKeySignatureV2) setAuthorizeIdentity(req *pb.AuthorizeRequest) {
	req.Identity = &pb.AuthorizeRequest_Signature{Signature: a.toAccessKeySignature()}
}

func (a AccessKeySignatureV2) setBulkAuthorizeIdentity(req *pb.BulkAuthorizeRequest) {
	req.Identity = &pb.BulkAuthorizeRequest_Signature{Signature: a.toAccessKeySignature()}
}

var _ Authorizable = (*AccessKeySignatureV4)(nil)

type AccessKeySignatureV4 struct {
	KeySign  accessKeySignature
	SignedAt time.Time
	Service  string
	Region   string
}

func (a AccessKeySignatureV4) hashingString() string {
	// hashingString must use all fields from Authorizable implementation. If someone add new field or change
	// something to a, compilation error will be raised.
	var _ struct {
		KeySign  accessKeySignature
		SignedAt time.Time
		Service  string
		Region   string
	} = a
	return a.KeySign.hashingString() + "<" + a.SignedAt.String() + "><" + a.Service + "><" + a.Region + ">"
}

func (a AccessKeySignatureV4) toAccessKeySignature() *pb.AccessKeySignature {
	aks := a.KeySign.toAccessKeySignature()
	aks.Parameters = &pb.AccessKeySignature_V4Parameters{
		V4Parameters: &pb.AccessKeySignature_Version4Parameters{
			SignedAt: &timestamp.Timestamp{Seconds: a.SignedAt.Unix()},
			Service:  a.Service,
			Region:   a.Region,
		},
	}
	return aks
}

func (a AccessKeySignatureV4) toAuthenticateRequest() *pb.AuthenticateRequest {
	return &pb.AuthenticateRequest{
		Credentials: &pb.AuthenticateRequest_Signature{Signature: a.toAccessKeySignature()},
	}
}

func (a AccessKeySignatureV4) setAuthorizeIdentity(req *pb.AuthorizeRequest) {
	req.Identity = &pb.AuthorizeRequest_Signature{Signature: a.toAccessKeySignature()}
}

func (a AccessKeySignatureV4) setBulkAuthorizeIdentity(req *pb.BulkAuthorizeRequest) {
	req.Identity = &pb.BulkAuthorizeRequest_Signature{Signature: a.toAccessKeySignature()}
}

var _ Authorizable = (*APIKey)(nil)

type APIKey struct {
	key string
}

func (k APIKey) hashingString() string {
	// hashingString must use all fields from Authorizable implementation. If someone add new field or change
	// something to k, compilation error will be raised.
	var _ struct {
		key string
	} = k
	return k.key
}

func (k APIKey) toAuthenticateRequest() *pb.AuthenticateRequest {
	return &pb.AuthenticateRequest{
		Credentials: &pb.AuthenticateRequest_ApiKey{ApiKey: k.key},
	}
}

func (k APIKey) setAuthorizeIdentity(req *pb.AuthorizeRequest) {
	req.Identity = &pb.AuthorizeRequest_ApiKey{ApiKey: k.key}
}

func (k APIKey) setBulkAuthorizeIdentity(req *pb.BulkAuthorizeRequest) {
	req.Identity = &pb.BulkAuthorizeRequest_ApiKey{ApiKey: k.key}
}

var _ Authenticatable = (*IAMCookie)(nil)

type IAMCookie struct {
	cookie string
}

func (k IAMCookie) toAuthenticateRequest() *pb.AuthenticateRequest {
	return &pb.AuthenticateRequest{
		Credentials: &pb.AuthenticateRequest_IamCookie{IamCookie: k.cookie},
	}
}

func makeAccessKeySignature(keyID, stringToSign, signature string) accessKeySignature {
	return accessKeySignature{
		KeyID:        keyID,
		StringToSign: stringToSign,
		Signature:    signature,
	}
}

func NewAccessKeySignatureV2(keyID, stringToSign, signature string,
	method pb.AccessKeySignature_Version2Parameters_SignatureMethod) *AccessKeySignatureV2 {
	return &AccessKeySignatureV2{
		KeySign:         makeAccessKeySignature(keyID, stringToSign, signature),
		SignatureMethod: method,
	}
}

func NewAccessKeySignatureV4(keyID, stringToSign, signature, service, region string, signedAt time.Time) *AccessKeySignatureV4 {
	return &AccessKeySignatureV4{
		KeySign:  makeAccessKeySignature(keyID, stringToSign, signature),
		SignedAt: signedAt,
		Service:  service,
		Region:   region,
	}
}

func NewIAMToken(token string) IAMToken {
	return IAMToken{Token: token}
}

func NewAPIKey(key string) APIKey {
	return APIKey{key: key}
}

func NewIAMCookie(key string) IAMCookie {
	return IAMCookie{cookie: key}
}

func NewAccessServiceClient(cc *grpc.ClientConn) *DefaultAccessServiceClient {
	return &DefaultAccessServiceClient{pb.NewAccessServiceClient(cc)}
}

// Allows creation from YCP SDK client.
func FromGRPCClient(a pb.AccessServiceClient) *DefaultAccessServiceClient {
	return &DefaultAccessServiceClient{client: a}
}

func (acc *DefaultAccessServiceClient) Authenticate(ctx context.Context, a Authenticatable) (Subject, error) {
	res, err := acc.client.Authenticate(ctx, a.toAuthenticateRequest())
	if err != nil {
		return nil, err
	}
	return subjectFromGrpc(res.Subject), nil
}

func (acc *DefaultAccessServiceClient) Authorize(ctx context.Context, a Authorizable, permission string, resourcePath ...Resource) (Subject, error) {
	if len(resourcePath) == 0 {
		return nil, errors.New("at least one resource is required")
	}
	req := makeAuthorizeRequest(permission, resourcePath...)
	a.setAuthorizeIdentity(req)
	res, err := acc.client.Authorize(ctx, req)
	if err != nil {
		return nil, err
	}
	return subjectFromGrpc(res.Subject), nil
}

func (acc *DefaultAccessServiceClient) BulkAuthorize(ctx context.Context, request *BulkAuthorizeRequest) (*BulkAuthorizeResponse, error) {
	req := bulkAuthorizeRequestToGrpc(request)
	res, err := acc.client.BulkAuthorize(ctx, req)
	if err != nil {
		return nil, err
	}
	return bulkAuthorizeResponseFromGrpc(res), nil
}

func subjectFromGrpc(subject *pb.Subject) Subject {
	switch subjectType := subject.Type.(type) {
	case *pb.Subject_AnonymousAccount_:
		return AnonymousAccount{}
	case *pb.Subject_UserAccount_:
		return UserAccount{subjectType.UserAccount.Id}
	case *pb.Subject_ServiceAccount_:
		return ServiceAccount{subjectType.ServiceAccount.Id, subjectType.ServiceAccount.FolderId}
	}
	return nil
}

func makeAuthorizeRequest(permission string, resourcePath ...Resource) *pb.AuthorizeRequest {
	grpcResources := make([]*pb.Resource, len(resourcePath))
	for i, res := range resourcePath {
		grpcResources[i] = &pb.Resource{
			Id:   res.ID,
			Type: res.Type,
		}
	}
	return &pb.AuthorizeRequest{
		Permission:   permission,
		ResourcePath: grpcResources,
	}
}

func bulkAuthorizeRequestToGrpc(request *BulkAuthorizeRequest) *pb.BulkAuthorizeRequest {
	var mask *fieldmaskpb.FieldMask
	if request.FieldMask != nil {
		paths := make([]string, len(request.FieldMask))
		for i, field := range request.FieldMask {
			paths[i] = string(field)
		}
		mask = &fieldmaskpb.FieldMask{Paths: paths}
	}

	result := &pb.BulkAuthorizeRequest{
		ResultFilter: pb.BulkAuthorizeRequest_ResultFilter(request.ResultFilter),
		ResultMask:   mask,
	}

	request.Authorizable.setBulkAuthorizeIdentity(result)

	if request.Actions != nil {
		if request.ActionMatrix != nil {
			panic("request should contain only Actions or only ActionMatrix")
		}
		result.Authorizations = &pb.BulkAuthorizeRequest_Actions_{
			Actions: actionsToGrpc(request.Actions),
		}
	} else {
		if request.ActionMatrix == nil {
			panic("request should contain either Actions or only ActionMatrix")
		}
		result.Authorizations = &pb.BulkAuthorizeRequest_ActionMatrix_{
			ActionMatrix: actionMatrixToGrpc(request.ActionMatrix),
		}
	}

	return result
}

func actionMatrixToGrpc(matrix *ActionMatrix) *pb.BulkAuthorizeRequest_ActionMatrix {
	resourcePaths := make([]*pb.ResourcePath, 0, len(matrix.ResourcePaths))
	for _, resourcePath := range matrix.ResourcePaths {
		resourcePaths = append(resourcePaths,
			&pb.ResourcePath{Path: resourcePathToGrpc(resourcePath)})
	}
	return &pb.BulkAuthorizeRequest_ActionMatrix{
		ResourcePaths: resourcePaths,
		Permissions:   matrix.Permissions,
	}
}

func actionsToGrpc(actions []*Action) *pb.BulkAuthorizeRequest_Actions {
	items := make([]*pb.BulkAuthorizeRequest_Action, 0, len(actions))
	for _, action := range actions {
		items = append(items, &pb.BulkAuthorizeRequest_Action{
			ResourcePath: resourcePathToGrpc(action.ResourcePath),
			Permission:   action.Permission,
		})
	}
	return &pb.BulkAuthorizeRequest_Actions{
		Items: items,
	}
}

func resourcePathToGrpc(resourcePath []*Resource) []*pb.Resource {
	result := make([]*pb.Resource, 0, len(resourcePath))
	for _, resource := range resourcePath {
		result = append(result, resourceToGrpc(resource))
	}
	return result
}

func resourceToGrpc(resource *Resource) *pb.Resource {
	return &pb.Resource{
		Id:   resource.ID,
		Type: resource.Type,
	}
}

func bulkAuthorizeResponseFromGrpc(res *pb.BulkAuthorizeResponse) *BulkAuthorizeResponse {
	results := make([]*BulkAuthorizeResult, 0, len(res.Results.Items))
	for _, item := range res.Results.Items {
		results = append(results, resultFromGrpc(item))
	}
	return &BulkAuthorizeResponse{
		Subject:              subjectFromGrpc(res.Subject),
		Results:              results,
		UnauthenticatedError: errorFromGrpc(res.GetUnauthenticatedError()),
	}
}

func resultFromGrpc(result *pb.BulkAuthorizeResponse_Result) *BulkAuthorizeResult {
	return &BulkAuthorizeResult{
		Permission:            result.Permission,
		ResourcePath:          resourcePathFromGrpc(result.ResourcePath),
		PermissionDeniedError: errorFromGrpc(result.GetPermissionDeniedError()),
	}
}

func errorFromGrpc(error *pb.BulkAuthorizeResponse_Error) (message string) {
	if error != nil {
		message = error.Message
	}
	return
}

func resourcePathFromGrpc(resourcePath []*pb.Resource) []*Resource {
	result := make([]*Resource, 0, len(resourcePath))
	for _, resource := range resourcePath {
		result = append(result, resourceFromGrpc(resource))
	}
	return result
}

func resourceFromGrpc(resource *pb.Resource) *Resource {
	return &Resource{
		ID:   resource.Id,
		Type: resource.Type,
	}
}
