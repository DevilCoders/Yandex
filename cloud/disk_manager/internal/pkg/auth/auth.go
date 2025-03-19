package auth

import (
	"context"
	"crypto/md5"
	"encoding/hex"
	"fmt"
	"strings"
	"time"

	grpc_retry "github.com/grpc-ecosystem/go-grpc-middleware/retry"
	"github.com/karlseguin/ccache/v2"
	ydb_credentials "github.com/ydb-platform/ydb-go-sdk/v3/credentials"
	ydb_log "github.com/ydb-platform/ydb-go-sdk/v3/log"
	yc "github.com/ydb-platform/ydb-go-yc-metadata"
	yc_trace "github.com/ydb-platform/ydb-go-yc-metadata/trace"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/keepalive"
	"google.golang.org/grpc/metadata"

	auth_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/auth/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
)

////////////////////////////////////////////////////////////////////////////////

const (
	tokenPrefix = "Bearer "
)

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

func getIAMToken(ctx context.Context) (string, error) {
	token := headers.GetAuthorizationHeader(ctx)

	if len(token) == 0 {
		return "", fmt.Errorf("failed to find auth token in the request")
	}

	if !strings.HasPrefix(token, tokenPrefix) {
		return "", fmt.Errorf(
			"expected token to start with \"%s\", found \"%.7s\"",
			tokenPrefix,
			token,
		)
	}

	return token[len(tokenPrefix):], nil
}

////////////////////////////////////////////////////////////////////////////////

func getTransportCredentials(
	certFile string,
) (credentials.TransportCredentials, error) {

	if len(certFile) == 0 {
		return credentials.NewClientTLSFromCert(nil, ""), nil
	}

	return credentials.NewClientTLSFromFile(certFile, "")
}

////////////////////////////////////////////////////////////////////////////////

type AccessServiceClient interface {
	Authorize(ctx context.Context, permission string) (string, error)
}

////////////////////////////////////////////////////////////////////////////////

type cachedKey struct {
	token      string
	permission string
}

func (k *cachedKey) Hash() (string, error) {
	h := md5.New()

	_, err := h.Write([]byte(k.token))
	if err != nil {
		return "", err
	}

	_, err = h.Write([]byte(k.permission))
	if err != nil {
		return "", err
	}

	return hex.EncodeToString(h.Sum(nil)), nil
}

type accessServiceClient struct {
	client   *cloudauth.AccessServiceClient
	resource cloudauth.Resource

	cacheLifetime time.Duration
	authCache     *ccache.Cache
}

type requestIDKey struct{}

func (c *accessServiceClient) Authorize(
	ctx context.Context,
	permission string,
) (string, error) {

	token, err := getIAMToken(ctx)
	if err != nil {
		return "", err
	}

	cacheKey := cachedKey{
		token:      token,
		permission: permission,
	}

	hash, err := cacheKey.Hash()
	if err == nil {
		item := c.authCache.Get(hash)
		if item != nil && !item.Expired() {
			return item.Value().(string), nil
		}
	}

	ctx = context.WithValue(
		ctx,
		requestIDKey{},
		headers.GetRequestID(ctx),
	)
	subj, err := c.client.Authorize(
		ctx,
		cloudauth.IAMToken(token),
		permission,
		c.resource,
	)
	if err != nil {
		return "", err
	}

	accountID, err := getAccountIDFromSubject(subj)
	if err != nil {
		return "", err
	}

	hash, err = cacheKey.Hash()
	if err == nil {
		c.authCache.Set(hash, accountID, c.cacheLifetime)
	}

	return accountID, nil
}

////////////////////////////////////////////////////////////////////////////////

type disabledClient struct{}

func (c *disabledClient) Authorize(
	ctx context.Context,
	permission string,
) (string, error) {

	return "unauthorized", nil
}

////////////////////////////////////////////////////////////////////////////////

func NewAccessServiceClientWithCreds(
	config *auth_config.AuthConfig,
	creds Credentials,
) (AccessServiceClient, error) {

	if config.GetDisableAuthorization() {
		return &disabledClient{}, nil
	}

	connectionTimeout, err := time.ParseDuration(config.GetConnectionTimeout())
	if err != nil {
		return nil, fmt.Errorf("failed to parse ConnectionTimeout: %w", err)
	}

	perRetryTimeout, err := time.ParseDuration(config.GetPerRetryTimeout())
	if err != nil {
		return nil, fmt.Errorf("failed to parse PerRetryTimeout: %w", err)
	}

	keepaliveTime, err := time.ParseDuration(config.GetKeepAliveTime())
	if err != nil {
		return nil, fmt.Errorf("failed to parse KeepAliveTime: %w", err)
	}

	keepaliveTimeout, err := time.ParseDuration(config.GetKeepAliveTimeout())
	if err != nil {
		return nil, fmt.Errorf("failed to parse KeepAliveTimeout: %w", err)
	}

	ctx, cancel := context.WithTimeout(context.Background(), connectionTimeout)
	defer cancel()

	retryInterceptor := grpc_retry.UnaryClientInterceptor(
		grpc_retry.WithMax(uint(config.GetRetryCount())),
		grpc_retry.WithPerRetryTimeout(perRetryTimeout),
	)

	interceptor := func(
		ctx context.Context,
		method string,
		req interface{},
		reply interface{},
		cc *grpc.ClientConn,
		invoker grpc.UnaryInvoker,
		opts ...grpc.CallOption) error {

		ctx = metadata.AppendToOutgoingContext(
			ctx,
			cloudauth.RequestID,
			ctx.Value(requestIDKey{}).(string),
		)

		if creds != nil {
			token, err := creds.Token(ctx)
			if err != nil {
				return err
			}

			ctx = headers.SetOutgoingAccessToken(ctx, token)
		}

		return retryInterceptor(ctx, method, req, reply, cc, invoker, opts...)
	}

	transportCreds, err := getTransportCredentials(config.GetCertFile())
	if err != nil {
		return nil, fmt.Errorf("failed to create credentials: %w", err)
	}

	conn, err := grpc.DialContext(
		ctx,
		config.GetAccessServiceEndpoint(),
		grpc.WithBlock(),
		grpc.WithTransportCredentials(transportCreds),
		grpc.WithUnaryInterceptor(interceptor),
		grpc.WithUserAgent("yc-disk-manager"),
		grpc.WithKeepaliveParams(keepalive.ClientParameters{
			Time:                keepaliveTime,
			Timeout:             keepaliveTimeout,
			PermitWithoutStream: true,
		}),
	)
	if err != nil {
		return nil, err
	}

	cacheLifetime, err := time.ParseDuration(
		config.GetAuthorizationCacheLifetime(),
	)

	if err != nil {
		return nil, err
	}

	return &accessServiceClient{
		client:        cloudauth.NewAccessServiceClient(conn),
		resource:      cloudauth.ResourceFolder(config.GetFolderId()),
		cacheLifetime: cacheLifetime,
		authCache:     ccache.New(ccache.Configure()),
	}, nil
}

func NewAccessServiceClient(config *auth_config.AuthConfig) (AccessServiceClient, error) {
	return NewAccessServiceClientWithCreds(config, nil)
}

////////////////////////////////////////////////////////////////////////////////

type Credentials = ydb_credentials.Credentials

type credentialsWrapper struct {
	impl ydb_credentials.Credentials
}

func (c *credentialsWrapper) Token(ctx context.Context) (string, error) {
	token, err := c.impl.Token(ctx)
	if err != nil {
		// Ignore token errors.
		return "", &errors.RetriableError{Err: err}
	}

	return token, nil
}

func NewCredentials(ctx context.Context, config *auth_config.AuthConfig) Credentials {
	if len(config.GetMetadataUrl()) == 0 {
		return nil
	}

	return &credentialsWrapper{
		impl: yc.NewInstanceServiceAccount(
			yc.WithURL(config.GetMetadataUrl()),
			yc.WithTrace(yc_trace.Trace{
				OnRefreshToken: func(info yc_trace.RefreshTokenStartInfo) func(yc_trace.RefreshTokenDoneInfo) {
					return func(info yc_trace.RefreshTokenDoneInfo) {
						if info.Error == nil {
							logging.Info(ctx, "token refresh done (token: %s, expiresIn: %v)", ydb_log.Secret(info.Token), info.ExpiresIn)
						} else {
							logging.Error(ctx, "token refresh fail: %v", info.Error)
						}
					}
				},
			}),
		),
	}
}
