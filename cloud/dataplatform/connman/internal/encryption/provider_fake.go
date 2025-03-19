package encryption

import (
	"context"
)

type fakeProvider struct{}

func NewFakeProvider() Provider {
	return &fakeProvider{}
}

func (p *fakeProvider) Encrypt(ctx context.Context, plaintext []byte) ([]byte, error) {
	return plaintext, nil
}

func (p *fakeProvider) Decrypt(ctx context.Context, ciphertext []byte) ([]byte, error) {
	return ciphertext, nil
}
