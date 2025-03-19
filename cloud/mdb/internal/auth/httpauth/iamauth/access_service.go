package iamauth

import (
	"context"
	"net/http"
	"strings"

	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	grpcas "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Config is IAM auth config
type Config struct {
	Host             string                `json:"host" yaml:"host"`
	CloudID          string                `json:"cloud_id" yaml:"cloud_id"`
	Permission       string                `json:"permission" yaml:"permission"`
	Client           string                `json:"client" yaml:"client"`
	GRPCClientConfig grpcutil.ClientConfig `json:"grpc" yaml:"grpc"`
}

// DefaultConfig ...
func DefaultConfig() Config {
	return Config{
		Host:    "as.cloud.yandex-team.ru:4286",
		CloudID: "fooe9dt3lm4bvrup1las",
		GRPCClientConfig: grpcutil.ClientConfig{
			Security: grpcutil.SecurityConfig{
				TLS: grpcutil.TLSConfig{
					CAFile: "/opt/yandex/allCAs.pem",
				},
			},
		},
	}
}

type Authenticator struct {
	as         accessservice.AccessService
	resource   cloudauth.Resource
	permission string
	l          log.Logger
}

var _ httpauth.Authenticator = &Authenticator{}

func New(ctx context.Context, cfg Config, l log.Logger) (*Authenticator, error) {
	as, err := grpcas.NewClient(ctx, cfg.Host, "Access Service Client", cfg.GRPCClientConfig, l)
	if err != nil {
		return nil, xerrors.Errorf("failed to init access service authentication: %w", err)
	}
	return &Authenticator{
		as:         as,
		resource:   cloudauth.ResourceCloud(cfg.CloudID),
		permission: cfg.Permission,
		l:          l,
	}, nil
}

func (a *Authenticator) Auth(ctx context.Context, request *http.Request) error {
	const prefix = "Bearer "
	header := request.Header.Get("Authorization")
	if header == "" {
		return httpauth.ErrNoAuthCredentials.Wrap(xerrors.New("'Authorization' header missing"))
	}
	iamToken := strings.TrimPrefix(header, prefix)
	if iamToken == header {
		return httpauth.ErrNoAuthCredentials.Wrap(xerrors.Errorf("'Authorization' header doesn't start with %q", prefix))
	}
	subject, err := a.as.Auth(ctx, iamToken, a.permission, a.resource)
	if err != nil {
		ctxlog.Errorf(
			ctxlog.WithFields(ctx,
				log.String("permission", a.permission),
				log.String("resource id", a.resource.ID),
				log.Error(err)),
			a.l,
			"Cannot authenticate with IAM")
		return httpauth.ErrAuthFailure.Wrap(err)
	}
	if subject.IsEmpty() {
		ctxlog.Warn(ctx, a.l, "iam subject is empty")
		return httpauth.ErrAuthNoRights.Wrap(xerrors.New("subject is empty"))
	}
	return nil // successful IAM auth
}

func (a *Authenticator) Ping(context.Context) error {
	return nil
}
