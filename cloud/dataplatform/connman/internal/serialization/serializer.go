package serialization

import (
	"context"

	"google.golang.org/protobuf/proto"

	"a.yandex-team.ru/cloud/dataplatform/connman/internal/encryption"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Serializer interface {
	Serialize(ctx context.Context, serializable Serializable) error
	Deserialize(ctx context.Context, serializable Serializable) error
}

type serializer struct {
	encryptionProvider encryption.Provider
}

func NewSerializer(encryptionProvider encryption.Provider) Serializer {
	return &serializer{encryptionProvider: encryptionProvider}
}

func (s *serializer) Serialize(ctx context.Context, serializable Serializable) error {
	plaintext, err := proto.Marshal(serializable.GetProtoMessage())
	if err != nil {
		return xerrors.Errorf("unable to marshal proto: %w", err)
	}

	ciphertext, err := s.encryptionProvider.Encrypt(ctx, plaintext)
	if err != nil {
		return xerrors.Errorf("unable to encrypt proto data: %w", err)
	}

	serializable.SetData(ciphertext)
	return nil
}

func (s *serializer) Deserialize(ctx context.Context, serializable Serializable) error {
	plaintext, err := s.encryptionProvider.Decrypt(ctx, serializable.GetData())
	if err != nil {
		return xerrors.Errorf("unable to decrypt proto data: %w", err)
	}

	err = proto.Unmarshal(plaintext, serializable.GetProtoMessage())
	if err != nil {
		return xerrors.Errorf("unable to unmarshal proto: %w", err)
	}

	return nil
}
