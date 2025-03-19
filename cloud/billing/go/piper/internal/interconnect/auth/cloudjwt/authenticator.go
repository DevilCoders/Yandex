package cloudjwt

import (
	"context"
	"crypto/rsa"
	"sync"
	"time"

	"github.com/golang-jwt/jwt/v4"
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/auth"
	iam "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1"
)

type Authenticator struct {
	tokenClient iam.IamTokenServiceClient

	audience   string
	accountID  string
	keyID      string
	privateKey *rsa.PrivateKey

	token          string
	expireDate     time.Time
	expireDeadline time.Time
	refreshDate    time.Time

	ctx context.Context

	mu sync.Mutex
}

type Config struct {
	Audience   Audience
	AccountID  string
	KeyID      string
	PrivateKey []byte
}

func New(ctx context.Context, conn grpc.ClientConnInterface, config Config) (*Authenticator, error) {
	privateKey, err := jwt.ParseRSAPrivateKeyFromPEM(config.PrivateKey)
	if err != nil {
		return nil, err
	}
	return &Authenticator{
		tokenClient: iam.NewIamTokenServiceClient(conn),
		audience:    string(config.Audience),
		accountID:   config.AccountID,
		keyID:       config.KeyID,
		privateKey:  privateKey,

		ctx: ctx,
	}, nil
}

func (t *Authenticator) GRPCAuth() auth.GRPCAuthenticator {
	return &auth.GRPCTokenAuthenticator{TokenGetter: t}
}

func (t *Authenticator) YDBAuth() auth.YDBAuthenticator {
	return &auth.YDBTokenAuthenticator{TokenGetter: t}
}

type Audience string
