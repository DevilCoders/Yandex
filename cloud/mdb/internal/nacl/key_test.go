package nacl_test

import (
	"crypto/rand"
	"testing"

	"github.com/stretchr/testify/require"
	"golang.org/x/crypto/nacl/box"

	"a.yandex-team.ru/cloud/mdb/internal/nacl"
)

func TestParseKey(t *testing.T) {
	publicKey, privateKey, _ := box.GenerateKey(rand.Reader)
	publicKeyEncoded := nacl.EncodeKey(*publicKey)
	privateKeyEncoded := nacl.EncodeKey(*privateKey)

	pubKeyParsed, err := nacl.ParseKey(publicKeyEncoded)
	require.NoError(t, err)
	require.Equal(t, *publicKey, pubKeyParsed)

	privateKeyParsed, err := nacl.ParseKey(privateKeyEncoded)
	require.NoError(t, err)
	require.Equal(t, *privateKey, privateKeyParsed)
}
