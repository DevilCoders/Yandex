package nacl_test

import (
	"crypto/rand"
	"testing"

	"github.com/stretchr/testify/require"
	"golang.org/x/crypto/nacl/box"

	"a.yandex-team.ru/cloud/mdb/internal/nacl"
)

func TestBox_Open(t *testing.T) {
	originalMsg := "valid_key"

	bobPublicKey, bobPrivateKey, err := box.GenerateKey(rand.Reader)
	require.NoError(t, err)
	alicePublicKey, alicePrivateKey, err := box.GenerateKey(rand.Reader)
	require.NoError(t, err)

	bobBox := nacl.Box{
		PeersPublicKey: *alicePublicKey,
		PrivateKey:     *bobPrivateKey,
	}

	aliceBox := nacl.Box{
		PeersPublicKey: *bobPublicKey,
		PrivateKey:     *alicePrivateKey,
	}

	encryptedMsg, err := bobBox.Seal([]byte(originalMsg))
	require.NoError(t, err)

	decryptedMsg, success := aliceBox.Open(encryptedMsg)
	require.True(t, success, "unsuccessful decryption")

	require.Equal(t, originalMsg, string(decryptedMsg))
}
