package globalauth

import (
	"context"

	"github.com/ydb-platform/ydb-go-yc-metadata"
	"go.uber.org/zap"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
)

const (
	MetadataTokenPolicy = "metadata"
	NoneTokenPolicy     = "none"
	metadataURL         = "http://localhost:6770/computeMetadata/v1/instance/service-accounts/default/token"

	authHeader = "authorization"
)

type TokenProvider interface {
	Token(ctx context.Context) (string, error)
}

type singleTokenProvider struct {
	token string
}

func (n *singleTokenProvider) Token(ctx context.Context) (string, error) {
	return n.token, nil
}

func NewSingleTokenProvider(token string) TokenProvider {
	return &singleTokenProvider{token: token}
}

// has to be thread-safe, could be nil (to mark absent credentials)
var credentials TokenProvider

// initialize local credentials object.
func InitCredentials(ctx context.Context, policy string) {
	switch policy {
	case MetadataTokenPolicy:
		credentials = yc.NewInstanceServiceAccount(yc.WithURL(metadataURL))
	case NoneTokenPolicy:
		credentials = nil
	default:
		ctxlog.G(ctx).Panic("impossible token policy found", zap.String("policy", policy))
	}
}

// singleton local metadata tokenizer object
func GetCredentials() TokenProvider {
	return credentials
}

type GRPCTokenProvider struct {
	TokenProvider
	Secure bool
}

func (p *GRPCTokenProvider) GetRequestMetadata(ctx context.Context, _ ...string) (map[string]string, error) {
	token, err := p.Token(ctx)
	if err != nil {
		return nil, err
	}
	return map[string]string{authHeader: token}, nil
}

func (p *GRPCTokenProvider) RequireTransportSecurity() bool {
	return p.Secure
}
