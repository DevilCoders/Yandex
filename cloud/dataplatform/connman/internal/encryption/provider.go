package encryption

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/kms/v1"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/configuration"
	"a.yandex-team.ru/library/go/core/xerrors"
	ycsdk "a.yandex-team.ru/transfer_manager/go/pkg/yc/sdk"
)

type Encryptor interface {
	Encrypt(ctx context.Context, plaintext []byte) ([]byte, error)
}

type Decryptor interface {
	Decrypt(ctx context.Context, ciphertext []byte) ([]byte, error)
}

type Provider interface {
	Encryptor
	Decryptor
}

type kmsProvider struct {
	client kms.SymmetricCryptoServiceClient
	keyID  string
}

func NewKmsProvider(ctx context.Context, config *configuration.Config) (Provider, error) {
	sdk, err := ycsdk.Instance()
	if err != nil {
		return nil, xerrors.Errorf("unable to get yc sdk instance: %w", err)
	}
	client, err := sdk.KmsSymmetricCryptoServiceClient(ctx)
	if err != nil {
		return nil, xerrors.Errorf("unable to get kms symmetric crypto service client: %w", err)
	}
	return &kmsProvider{client: client, keyID: config.KMSKeyID}, nil
}

func (p *kmsProvider) Encrypt(ctx context.Context, plaintext []byte) ([]byte, error) {
	request := &kms.SymmetricEncryptRequest{KeyId: p.keyID, Plaintext: plaintext}
	response, err := p.client.Encrypt(ctx, request)
	if err != nil {
		return nil, xerrors.Errorf("unable to encrypt: %w", err)
	}
	return response.Ciphertext, nil
}

func (p *kmsProvider) Decrypt(ctx context.Context, ciphertext []byte) ([]byte, error) {
	request := &kms.SymmetricDecryptRequest{KeyId: p.keyID, Ciphertext: ciphertext}
	response, err := p.client.Decrypt(ctx, request)
	if err != nil {
		return nil, xerrors.Errorf("unable to decrypt: %w", err)
	}
	return response.Plaintext, nil
}
