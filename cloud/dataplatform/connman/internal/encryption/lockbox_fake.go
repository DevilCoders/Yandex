package encryption

import (
	"context"
)

type fakeLockboxDecryptor struct{}

func NewFakeLockboxDecryptor() LockboxDecryptor {
	return &fakeLockboxDecryptor{}
}

func (l *fakeLockboxDecryptor) Decrypt(ctx context.Context, secretID string) (string, error) {
	return secretID, nil
}
