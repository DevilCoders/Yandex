package types_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

func TestKeys(t *testing.T) {
	priv, err := crypto.GeneratePrivateKey()
	require.NoError(t, err)
	require.NotNil(t, priv)

	msg := []byte("my secret message")

	signature, err := priv.HashAndSign(msg)
	require.NoError(t, err)
	require.NotNil(t, signature)

	pub := priv.GetPublicKey()
	require.NotNil(t, pub)

	require.Error(t, pub.Verify(msg, signature))
	require.NoError(t, pub.HashAndVerify(msg, signature))
	require.Error(t, pub.HashAndVerify(append(msg, 0x20), signature))
	require.Error(t, pub.HashAndVerify(msg, append(signature, 0x20)))
}

func TestHostNeighboursInfo_IsHA(t *testing.T) {
	testCases := []struct {
		input  types.HostNeighboursInfo
		result bool
	}{
		{
			input: types.HostNeighboursInfo{
				HACluster:      true,
				HAShard:        false,
				SameRolesTotal: 2,
			},
			result: true,
		},
		{
			input: types.HostNeighboursInfo{
				HACluster:      false,
				HAShard:        true,
				SameRolesTotal: 1,
			},
			result: true,
		},
		{
			input: types.HostNeighboursInfo{
				HACluster:      false,
				HAShard:        false,
				SameRolesTotal: 2,
			},
			result: false,
		},
		{
			input: types.HostNeighboursInfo{
				HACluster:      true,
				HAShard:        true,
				SameRolesTotal: 0,
			},
			result: false,
		},
	}

	for _, tc := range testCases {
		t.Run("", func(t *testing.T) {
			require.Equal(t, tc.result, tc.input.IsHA())
		})
	}
}
