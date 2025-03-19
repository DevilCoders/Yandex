package keys

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/mdb-disklock/internal/keys/aws"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Key interface {
	GetKey(ctx context.Context) ([]byte, error)
	EncryptKey(ctx context.Context, plaintextKey []byte) error
	Name() string
}

func NewKey(cfg Config, l log.Logger, awsTransport httputil.TransportConfig) (Key, error) {
	switch cfg.Type {
	case aws.KeyType:
		return aws.NewKey(
			cfg.Name,
			cfg.Ciphertext,
			cfg.AWSConfig,
			l,
			awsTransport,
		)
	case PlaintextKeyType:
		return NewPlaintextKey(cfg), nil
	default:
		return nil, xerrors.Errorf("invalid key type %s", cfg.Type)
	}
}
