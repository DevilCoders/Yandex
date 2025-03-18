package oauth_test

import (
	"io/ioutil"
	"testing"

	"github.com/stretchr/testify/require"
	"golang.org/x/crypto/ssh"

	"a.yandex-team.ru/library/go/yandex/oauth"
)

func TestSSHFileKeyring(t *testing.T) {
	type TestCase struct {
		keyPath     string
		fingerprint string
		err         bool
	}

	cases := []TestCase{
		{
			keyPath:     "testdata/rsa",
			fingerprint: "SHA256:LUcyC9oREaX/EvmBoF4/Dvprx4DMPrvHBlK4Ls0w7T8",
			err:         false,
		},
		{
			keyPath:     "testdata/rsa.pub",
			fingerprint: "",
			err:         true,
		},
		{
			keyPath:     "testdata/ed25519",
			fingerprint: "SHA256:uFWTt+42ACob2Qd1w1woiykSzPcHidMYtHXEbIrALr4",
			err:         false,
		},
		{
			keyPath:     "testdata/ed25519.enc",
			fingerprint: "",
			err:         true,
		},
	}

	tester := func(tc TestCase) {
		t.Run(tc.keyPath, func(t *testing.T) {
			t.Parallel()

			keyring, err := oauth.NewSSHFileKeyring(tc.keyPath)
			if tc.err {
				require.Error(t, err)
				return
			}

			defer func() {
				err := keyring.Close()
				require.NoError(t, err)
			}()
			require.NoError(t, err)
			require.True(t, keyring.Next())
			require.NotNil(t, keyring.Signer())
			require.Equal(t, tc.fingerprint, ssh.FingerprintSHA256(keyring.Signer().PublicKey()))
			require.False(t, keyring.Next())
		})
	}

	for _, tc := range cases {
		tester(tc)
	}
}

func TestSSHByteKeyring(t *testing.T) {
	type TestCase struct {
		keyPath     string
		fingerprint string
		err         bool
	}

	cases := []TestCase{
		{
			keyPath:     "testdata/rsa",
			fingerprint: "SHA256:LUcyC9oREaX/EvmBoF4/Dvprx4DMPrvHBlK4Ls0w7T8",
			err:         false,
		},
		{
			keyPath:     "testdata/rsa.pub",
			fingerprint: "",
			err:         true,
		},
		{
			keyPath:     "testdata/ed25519",
			fingerprint: "SHA256:uFWTt+42ACob2Qd1w1woiykSzPcHidMYtHXEbIrALr4",
			err:         false,
		},
		{
			keyPath:     "testdata/ed25519.enc",
			fingerprint: "",
			err:         true,
		},
	}

	tester := func(tc TestCase) {
		t.Run(tc.keyPath, func(t *testing.T) {
			t.Parallel()

			key, err := ioutil.ReadFile(tc.keyPath)
			require.NoError(t, err)

			keyring, err := oauth.NewSSHByteKeyring(key)
			if tc.err {
				require.Error(t, err)
				return
			}

			defer func() {
				err := keyring.Close()
				require.NoError(t, err)
			}()
			require.NoError(t, err)
			require.True(t, keyring.Next())
			require.NotNil(t, keyring.Signer())
			require.Equal(t, tc.fingerprint, ssh.FingerprintSHA256(keyring.Signer().PublicKey()))
			require.False(t, keyring.Next())
		})
	}

	for _, tc := range cases {
		tester(tc)
	}
}

func TestSSHContainerKeyring(t *testing.T) {
	byteKey := []byte(`
-----BEGIN OPENSSH PRIVATE KEY-----
b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAAAMwAAAAtzc2gtZW
QyNTUxOQAAACCxZoVg0eappgci8zdIjQ9MrOgWb7B37ZIg44jnTrdAYwAAAJBCdnH0QnZx
9AAAAAtzc2gtZWQyNTUxOQAAACCxZoVg0eappgci8zdIjQ9MrOgWb7B37ZIg44jnTrdAYw
AAAEAWV324G1eAWQx6oYt1VV6984enZUo6gts2GE6ugLYKvLFmhWDR5qmmByLzN0iND0ys
6BZvsHftkiDjiOdOt0BjAAAADHRlc3RfZWQyNTUxOQE=
-----END OPENSSH PRIVATE KEY-----
`)
	keyring0, _ := oauth.NewSSHFileKeyring("testdata/rsa")
	keyring1, _ := oauth.NewSSHFileKeyring("testdata/ed25519")
	keyring2, _ := oauth.NewSSHByteKeyring(byteKey)

	nestedKeyring := oauth.NewSSHContainerKeyring(keyring0, keyring1)
	keyring := oauth.NewSSHContainerKeyring(nestedKeyring, keyring2)

	require.True(t, keyring.Next())
	require.Equal(t, "SHA256:LUcyC9oREaX/EvmBoF4/Dvprx4DMPrvHBlK4Ls0w7T8", ssh.FingerprintSHA256(keyring.Signer().PublicKey()))
	require.Equal(t, "SHA256:LUcyC9oREaX/EvmBoF4/Dvprx4DMPrvHBlK4Ls0w7T8", ssh.FingerprintSHA256(keyring.Signer().PublicKey()))
	require.Equal(t, "SHA256:LUcyC9oREaX/EvmBoF4/Dvprx4DMPrvHBlK4Ls0w7T8", ssh.FingerprintSHA256(keyring.Signer().PublicKey()))

	require.True(t, keyring.Next())
	require.Equal(t, "SHA256:uFWTt+42ACob2Qd1w1woiykSzPcHidMYtHXEbIrALr4", ssh.FingerprintSHA256(keyring.Signer().PublicKey()))

	require.True(t, keyring.Next())
	require.Equal(t, "SHA256:uFWTt+42ACob2Qd1w1woiykSzPcHidMYtHXEbIrALr4", ssh.FingerprintSHA256(keyring.Signer().PublicKey()))

	require.False(t, keyring.Next())
	require.NoError(t, keyring.Close())
}
