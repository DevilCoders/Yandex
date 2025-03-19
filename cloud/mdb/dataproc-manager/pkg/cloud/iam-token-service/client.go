package cloud

import (
	"context"
	"sync"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type IamTokenServiceConfig struct {
	CAPath            string        `json:"capath" yaml:"capath"`
	ServiceURL        string        `json:"service_url" yaml:"service_url"`
	GrpcTimeout       time.Duration `json:"grpc_timeout" yaml:"grpc_timeout"`
	TokenCacheTimeout time.Duration `json:"token_cache_timeout" yaml:"token_cache_timeout"`
}

// DefaultConfig describes configuration for grpc client
func DefaultConfig() IamTokenServiceConfig {
	return IamTokenServiceConfig{
		CAPath:            "/opt/yandex/allCAs.pem",
		ServiceURL:        "ts.private-api.cloud-preprod.yandex.net:4282",
		GrpcTimeout:       30 * time.Second,
		TokenCacheTimeout: 6 * time.Hour,
	}
}

type CachedToken struct {
	creationTime time.Time
	iamToken     string
}

type client struct {
	logger     log.Logger
	conn       *grpc.ClientConn
	connMutex  sync.Mutex
	config     IamTokenServiceConfig
	useragent  string
	tokenCache sync.Map
	creds      credentials.PerRPCCredentials
}

type IamTokenServiceClient interface {
	CreateForServiceAccount(ctx context.Context, serviceAccountID string) (string, error)
}

var _ IamTokenServiceClient = &client{}

// New constructs grpc client
// It is a thread safe.
func New(config IamTokenServiceConfig, creds credentials.PerRPCCredentials, logger log.Logger) IamTokenServiceClient {
	client := &client{
		config:    config,
		logger:    logger,
		useragent: "Dataproc-Manager",
		creds:     creds,
	}
	return client
}

func (c *client) getConnection(ctx context.Context) (*grpc.ClientConn, error) {
	if c.conn != nil {
		return c.conn, nil
	}

	c.connMutex.Lock()
	defer c.connMutex.Unlock()

	if c.conn != nil {
		return c.conn, nil
	}

	conn, err := grpcutil.NewConn(
		ctx,
		c.config.ServiceURL,
		c.useragent,
		grpcutil.ClientConfig{
			Security: grpcutil.SecurityConfig{
				TLS: grpcutil.TLSConfig{
					CAFile: c.config.CAPath,
				},
			},
			Retries: interceptors.ClientRetryConfig{
				PerRetryTimeout: c.config.GrpcTimeout,
			},
		},
		c.logger,
		grpcutil.WithClientCredentials(c.creds),
	)
	if err != nil {
		return nil, xerrors.Errorf("can not connect to URL %q: %w", c.config.ServiceURL, err)
	}
	c.conn = conn
	return conn, nil
}

func (c *client) CreateForServiceAccount(ctx context.Context, serviceAccountID string) (string, error) {
	value, ok := c.tokenCache.Load(serviceAccountID)
	if ok {
		cachedToken := value.(CachedToken)
		now := time.Now()
		elapsed := now.Sub(cachedToken.creationTime)
		if int(elapsed.Seconds()) < int(c.config.TokenCacheTimeout.Seconds()) {
			return cachedToken.iamToken, nil
		}
	}
	c.logger.Debugf("Requesting new IAM token for service account %+v", serviceAccountID)

	conn, err := c.getConnection(ctx)
	if err != nil {
		return "", err
	}
	grpcClient := iam.NewIamTokenServiceClient(conn)

	var createIamTokenResponse *iam.CreateIamTokenResponse
	createIamTokenResponse, err = grpcClient.CreateForServiceAccount(
		ctx,
		&iam.CreateIamTokenForServiceAccountRequest{ServiceAccountId: serviceAccountID},
	)
	if err != nil {
		return "", xerrors.Errorf("can not call method 'CreateForServiceAccount': %w", err)
	}
	iamToken := createIamTokenResponse.GetIamToken()
	c.tokenCache.Store(
		serviceAccountID,
		CachedToken{
			creationTime: time.Now(),
			iamToken:     iamToken,
		},
	)

	return iamToken, nil
}
