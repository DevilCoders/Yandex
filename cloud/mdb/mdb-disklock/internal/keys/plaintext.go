package keys

import (
	"context"
)

var (
	PlaintextKeyType = "plaintext"
)

type PlaintextKey struct {
	name string
	key  []byte
}

func (k *PlaintextKey) EncryptKey(_ context.Context, _ []byte) error {
	return nil
}

func (k *PlaintextKey) GetKey(_ context.Context) ([]byte, error) {
	return k.key, nil
}

func (k *PlaintextKey) Name() string {
	return k.name
}

var _ Key = &PlaintextKey{}

func NewPlaintextKey(cfg Config) Key {
	return &PlaintextKey{
		name: cfg.Name,
		key:  []byte(cfg.Ciphertext),
	}
}
