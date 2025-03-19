package main

import (
	"bytes"
	"context"
	"testing"
)

var (
	testEncryptKey = []byte{1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6}
)

func TestEncryption(t *testing.T) {
	source := make([]byte, 10000)
	for i := range source {
		source[i] = byte(i)
	}

	ctx := context.Background()
	for i := 0; i < len(source); i++ {
		encrypted := encrypt(ctx, testEncryptKey, source[:i])
		decrypted := decrypt(ctx, testEncryptKey, encrypted)
		if !bytes.Equal(source[:i], decrypted) {
			t.Error()
		}
		if len(encrypted) != i+16 {
			t.Error(len(encrypted), len(decrypted), len(encrypted)-len(decrypted))
		}
	}
}
