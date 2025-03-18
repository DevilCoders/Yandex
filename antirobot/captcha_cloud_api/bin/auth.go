package main

import (
	"context"
	"log"
	"strings"
	"time"

	audit_events_pb "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events"
	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	"a.yandex-team.ru/library/go/core/xerrors"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/keepalive"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/status"

	grpc_retry "github.com/grpc-ecosystem/go-grpc-middleware/retry"
)

const (
	CaptchaGetPermission       = "smart-captcha.captchas.get"
	CaptchaCreatePermission    = "smart-captcha.captchas.create"
	CaptchaUpdatePermission    = "smart-captcha.captchas.update"
	CaptchaDeletePermission    = "smart-captcha.captchas.delete"
	CaptchaShowPermission      = "smart-captcha.captchas.show"
	CaptchaUpdateAllPermission = "smart-captcha.captchas.updateAll"

	QuotaGetPermission    = "smart-captcha.quotas.get"
	QuotaUpdatePermission = "smart-captcha.quotas.updateLimit"
)

type Authorizeable interface {
	Authorize(ctx context.Context, a cloudauth.Authorizable, permission string, resourcePath ...cloudauth.Resource) (cloudauth.Subject, error)
	Authenticate(ctx context.Context, a cloudauth.Authenticatable) (cloudauth.Subject, error)
}

type EmptyAuthorizeable struct{}

func (e EmptyAuthorizeable) Authorize(ctx context.Context, a cloudauth.Authorizable, permission string, resourcePath ...cloudauth.Resource) (cloudauth.Subject, error) {
	return &cloudauth.UserAccount{ID: "test"}, nil
}

func (e EmptyAuthorizeable) Authenticate(ctx context.Context, a cloudauth.Authenticatable) (cloudauth.Subject, error) {
	return &cloudauth.UserAccount{ID: "test"}, nil
}

func NewAccessServiceClient(ctx context.Context, args *Args) (Authorizeable, error) {
	if args.SkipAuthorization {
		return EmptyAuthorizeable{}, nil
	}

	type contextKey string
	retryInterceptor := grpc_retry.UnaryClientInterceptor(
		grpc_retry.WithMax(7),
		grpc_retry.WithPerRetryTimeout(2*time.Second),
	)
	var requestIDInterceptor grpc.UnaryClientInterceptor = func(ctx context.Context, method string, req, reply interface{},
		cc *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {

		reqID := ctx.Value(contextKey("requestId"))
		if reqID != nil {
			_ = grpc.SetHeader(ctx, metadata.Pairs(cloudauth.RequestID, reqID.(string)))
		}
		return retryInterceptor(ctx, method, req, reply, cc, invoker, opts...)
	}

	log.Printf("Init access service: %v\n", args.IAMAccessServiceEndpoint)
	var transportCredentials grpc.DialOption
	if args.AccessServiceInsecure {
		transportCredentials = grpc.WithInsecure()
	} else {
		creds, err := credentials.NewClientTLSFromFile(args.YandexInternalRootCACertPath, "")
		if err != nil {
			return nil, xerrors.Errorf("Cannot create client transport credentials for access-service (credentials.NewClientTLSFromFile): %w", err)
		}
		transportCredentials = grpc.WithTransportCredentials(creds)
	}
	conn, err := grpc.DialContext(ctx, args.IAMAccessServiceEndpoint,
		transportCredentials,
		grpc.WithUnaryInterceptor(requestIDInterceptor),
		grpc.WithUserAgent("YC-CAPTCHA"),
		grpc.WithKeepaliveParams(keepalive.ClientParameters{
			Time:                10 * time.Second,
			Timeout:             1 * time.Second,
			PermitWithoutStream: true,
		}),
	)
	if err != nil {
		return nil, xerrors.Errorf("Cannot dial %s: %w", args.IAMAccessServiceEndpoint, err)
	}
	client := cloudauth.NewAccessServiceClient(conn)
	return client, nil
}

type Subject struct {
	Subject cloudauth.Subject
}

func (a *Subject) ToAuditAuthentication() *audit_events_pb.Authentication {
	switch subjectType := a.Subject.(type) {
	case cloudauth.ServiceAccount:
		return &audit_events_pb.Authentication{
			Authenticated: true,
			SubjectType:   audit_events_pb.Authentication_SERVICE_ACCOUNT,
			SubjectId:     subjectType.ID,
		}
	case cloudauth.UserAccount:
		return &audit_events_pb.Authentication{
			Authenticated: true,
			SubjectType:   audit_events_pb.Authentication_YANDEX_PASSPORT_USER_ACCOUNT,
			SubjectId:     subjectType.ID,
		}
	default:
		return &audit_events_pb.Authentication{
			Authenticated: false,
		}
	}
}

func (a *Subject) GetID() string {
	switch subjectType := a.Subject.(type) {
	case cloudauth.ServiceAccount:
		return subjectType.ID
	case cloudauth.UserAccount:
		return subjectType.ID
	default:
		return ""
	}
}

type Authority struct {
	Auth Authorizeable
}

func (a *Authority) Authenticate(ctx context.Context) (*Subject, error) {
	authenticatable, authenticatableErr := a.authenticatableFromContext(ctx)
	if authenticatableErr != nil {
		return nil, authenticatableErr
	}
	subject, err := a.Auth.Authenticate(ctx, authenticatable)
	if err != nil {
		return nil, err
	}
	return &Subject{Subject: subject}, nil
}

func (a *Authority) AuthorizeByIamToken(ctx context.Context, resource cloudauth.Resource, permission string) (*Subject, error) {
	authorizable, authorizableErr := a.authorizableFromContext(ctx)
	if authorizableErr != nil {
		return nil, authorizableErr
	}
	return a.AuthorizeBySubject(ctx, authorizable, resource, permission)
}

func (a *Authority) AuthorizeBySubject(ctx context.Context, authorizable cloudauth.Authorizable, resource cloudauth.Resource, permission string) (*Subject, error) {
	subject, err := a.Auth.Authorize(ctx, authorizable, permission, resource)
	if err != nil {
		return nil, err
	}
	return &Subject{Subject: subject}, nil
}

func (a *Authority) authenticatableFromContext(ctx context.Context) (cloudauth.Authenticatable, error) {
	iamToken := extractIAMToken(ctx)
	if iamToken == "" {
		switch a.Auth.(type) {
		case EmptyAuthorizeable:
			// do not return error for EmptyAuthorizeable
		default:
			return nil, status.Errorf(codes.Unauthenticated, "The client provided invalid token")
		}
	}
	return cloudauth.NewIAMToken(iamToken), nil
}

func (a *Authority) authorizableFromContext(ctx context.Context) (cloudauth.Authorizable, error) {
	iamToken := extractIAMToken(ctx)
	if iamToken == "" {
		switch a.Auth.(type) {
		case EmptyAuthorizeable:
			// do not return error for EmptyAuthorizeable
		default:
			return nil, status.Errorf(codes.Unauthenticated, "The client provided invalid token")
		}
	}
	return cloudauth.NewIAMToken(iamToken), nil
}

func extractIAMToken(ctx context.Context) string {
	headers, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		return ""
	}

	parts := headers["authorization"]
	if len(parts) == 0 {
		return ""
	}
	part := parts[0]

	const tokenPrefix = "bearer "

	if !strings.HasPrefix(strings.ToLower(part), tokenPrefix) {
		return ""
	}

	return strings.TrimLeft(part[len(tokenPrefix):], " ")
}
