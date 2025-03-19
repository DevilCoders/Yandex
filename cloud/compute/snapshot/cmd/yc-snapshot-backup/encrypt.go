package main

import (
	"bytes"
	"context"
	"crypto/aes"
	"crypto/cipher"
	"io"
	"math/rand"

	"go.uber.org/zap"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
)

func encrypt(ctx context.Context, secret, data []byte) []byte {
	block, err := aes.NewCipher(secret)
	if err != nil {
		ctxlog.G(ctx).Fatal("create aes block", zap.Error(err))
	}
	iv := make([]byte, block.BlockSize())
	rand.Read(iv)

	out := &bytes.Buffer{}
	out.Grow(len(data) + block.BlockSize())
	out.Write(iv)

	w := cipher.StreamWriter{
		S: cipher.NewCTR(block, iv),
		W: out,
	}
	_, _ = w.Write(data)
	return out.Bytes()
}

func decrypt(ctx context.Context, secret, data []byte) []byte {
	block, err := aes.NewCipher(secret)
	if err != nil {
		ctxlog.G(ctx).Fatal("create aes block", zap.Error(err))
	}
	iv := data[:block.BlockSize()]
	data = data[block.BlockSize():]
	r := cipher.StreamReader{
		S: cipher.NewCTR(block, iv),
		R: bytes.NewReader(data),
	}
	res := make([]byte, len(data))
	_, _ = io.ReadFull(r, res)
	return res
}
