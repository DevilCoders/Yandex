package pg

import (
	"crypto/rand"
	"encoding/base64"
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/require"
	"golang.org/x/crypto/nacl/box"

	"a.yandex-team.ru/cloud/mdb/internal/nacl"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/models"
)

func TestService_encryptKey(t *testing.T) {

	originalKey := "valid_key"

	// keys generation
	type keys struct {
		saltPublicKey        *[32]byte
		saltPrivateKey       *[32]byte
		mdbSecretsPrivateKey *[32]byte
		mdbSecretsPublicKey  *[32]byte
	}
	var k keys
	k.saltPublicKey, k.saltPrivateKey, _ = box.GenerateKey(rand.Reader)
	k.mdbSecretsPublicKey, k.mdbSecretsPrivateKey, _ = box.GenerateKey(rand.Reader)

	s := &serviceImpl{
		saltPublicKey: *k.saltPublicKey,
		privateKey:    *k.mdbSecretsPrivateKey,
	}

	encKey, err := s.encryptData(originalKey)
	require.NoError(t, err, "encryption error")

	// unmarshal result string
	em := models.EncryptedMessage{}
	require.NoError(t, json.Unmarshal(encKey, &em))

	// keys that salt will have
	saltBox := nacl.Box{
		PeersPublicKey: *k.mdbSecretsPublicKey,
		PrivateKey:     *k.saltPrivateKey,
	}

	byteEncKey, err := base64.URLEncoding.DecodeString(em.Data)
	require.NoError(t, err)

	decrypted, decRes := saltBox.Open(byteEncKey)

	require.True(t, decRes, "decryption failed")
	require.Equal(t, originalKey, string(decrypted))
}
