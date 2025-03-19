package auth

import (
	"context"
	"crypto/tls"
	"fmt"
	"strings"
	"time"

	"google.golang.org/grpc/credentials"

	grpc_retry "github.com/grpc-ecosystem/go-grpc-middleware/retry"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"google.golang.org/grpc"
	"google.golang.org/grpc/keepalive"
	"google.golang.org/grpc/metadata"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/pkg/xrequestid"
	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	authorizationHeader                = "authorization"
	authorizedHeaderForOldPythonClient = "yc-authorization"
	requestIDHeader                    = "x-request-id"
	authTokenPrefixGRPC                = "Bearer "
)

var (
	errTokenFormat = xerrors.New("wrong token format")
)

type Config struct {
	Address                  string
	DenyUnauthorizedRequests bool
	FolderID                 string
	InsecureConnection       bool
	OneCallTimeoutSeconds    int
	MaxRetryCount            uint
}

type AccessServiceClient interface {
	Authorize(ctx context.Context, permission string) (user string, err error)
}

type AccessServiceClientImpl struct {
	enableDenyUnauthorizedRequests bool
	client                         *cloudauth.AccessServiceClient
	folder                         cloudauth.Resource
}

func NewAccessServiceClient(ctx context.Context, config Config) (*AccessServiceClientImpl, error) {
	perRetryTimeout := time.Duration(config.OneCallTimeoutSeconds) * time.Second
	if perRetryTimeout == 0 {
		perRetryTimeout = time.Second
	}

	retryCount := config.MaxRetryCount
	if retryCount == 0 {
		retryCount = 3
	}

	keepaliveTime := time.Minute
	keepaliveTimeout := time.Minute

	retryInterceptor := grpc_retry.UnaryClientInterceptor(
		grpc_retry.WithMax(retryCount),
		grpc_retry.WithPerRetryTimeout(perRetryTimeout),
	)

	requestIDInterceptor := func(
		ctx context.Context,
		method string,
		req interface{},
		reply interface{},
		cc *grpc.ClientConn,
		invoker grpc.UnaryInvoker,
		opts ...grpc.CallOption) error {

		requestID, ok := xrequestid.FromContext(ctx)
		if ok {
			ctx = metadata.AppendToOutgoingContext(
				ctx,
				cloudauth.RequestID,
				requestID,
			)
		}
		return retryInterceptor(ctx, method, req, reply, cc, invoker, opts...)
	}

	var dialOptions = []grpc.DialOption{
		grpc.WithUnaryInterceptor(requestIDInterceptor),
		grpc.WithUserAgent("yc-snapshot"),
		grpc.WithKeepaliveParams(keepalive.ClientParameters{
			Time:                keepaliveTime,
			Timeout:             keepaliveTimeout,
			PermitWithoutStream: true,
		}),
	}

	if config.InsecureConnection {
		dialOptions = append(dialOptions, grpc.WithInsecure())
	} else {
		dialOptions = append(dialOptions, grpc.WithTransportCredentials(credentials.NewTLS(&tls.Config{})))
	}

	conn, err := grpc.DialContext(
		ctx,
		config.Address,
		dialOptions...,
	)

	ctxlog.DebugErrorCtx(ctx, err, "Start grpc connection pool")
	if err != nil {
		return nil, xerrors.Errorf("start grpc connection for access service: %w", err)
	}

	return &AccessServiceClientImpl{
		enableDenyUnauthorizedRequests: config.DenyUnauthorizedRequests,
		client:                         cloudauth.NewAccessServiceClient(conn),
		folder:                         cloudauth.ResourceFolder(config.FolderID),
	}, nil
}

func (as *AccessServiceClientImpl) Authorize(ctx context.Context, permission string) (user string, err error) {
	fail := func(err error) (string, error) {
		var logLevel = zapcore.ErrorLevel
		if !as.enableDenyUnauthorizedRequests {
			logLevel = zapcore.WarnLevel
		}
		ctxlog.LevelCtx(ctx, logLevel, "Authorize", zap.Error(err))
		if !as.enableDenyUnauthorizedRequests {
			return "unauthoized-stub", nil
		}
		return "", err
	}
	token := getIAMToken(ctx)
	if token == "" {
		return fail(xerrors.New("can't get iam token"))
	}
	if strings.HasPrefix(token, authTokenPrefixGRPC) {
		token = strings.TrimPrefix(token, authTokenPrefixGRPC)
	} else {
		return fail(xerrors.Errorf("Can't accept iam-token: (%s) %w", token, errTokenFormat))
	}

	subj, err := as.client.Authorize(ctx, cloudauth.NewIAMToken(token), permission, as.folder)
	if err != nil {
		return fail(xerrors.Errorf("authorize error: %w", err))
	}
	accID, err := getAccountIDFromSubject(subj)
	ctxlog.DebugErrorCtx(ctx, err, "Get account from access service")
	if err != nil {
		return "", xerrors.Errorf("authorize error: %w", err)
	}
	return accID, nil
}

func getIAMToken(ctx context.Context) string {
	metadata, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		return ""
	}
	key := metadata.Get(authorizationHeader)
	if len(key) == 0 {
		key = metadata.Get(authorizedHeaderForOldPythonClient)
		if len(key) == 0 {
			return ""
		}
		ctxlog.G(ctx).Debug("Got iam token from old python header")
	}
	return key[0]
}

func getAccountIDFromSubject(subject cloudauth.Subject) (string, error) {
	switch s := subject.(type) {
	case cloudauth.AnonymousAccount:
		return "", fmt.Errorf("cannot authorize anonymous account")
	case cloudauth.UserAccount:
		return fmt.Sprintf("user-%v", s.ID), nil
	case cloudauth.ServiceAccount:
		return fmt.Sprintf("svc-%v-%v", s.ID, s.FolderID), nil
	default:
		return "", fmt.Errorf("unknown subject %v", subject)
	}
}
