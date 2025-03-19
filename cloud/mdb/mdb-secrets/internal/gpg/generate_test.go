package gpg

import (
	"bytes"
	"io/ioutil"
	"strings"
	"testing"

	"github.com/stretchr/testify/require"
	"golang.org/x/crypto/openpgp"
)

func TestGenerateGPGOk(t *testing.T) {
	result, err := GenerateGPG("test_cid")
	require.NoError(t, err)

	_, err = openpgp.ReadArmoredKeyRing(strings.NewReader(result))
	require.NoError(t, err)
}

func TestGenerateGPGDefaultEncrypt(t *testing.T) {
	result, err := GenerateGPG("test_cid")
	require.NoError(t, err)

	const origMsg = "origMsg"

	el, err := openpgp.ReadArmoredKeyRing(strings.NewReader(result))
	require.NoError(t, err)

	buf := &bytes.Buffer{}
	wc, err := openpgp.Encrypt(buf, el, nil, nil, nil)
	require.NoError(t, err)

	_, err = wc.Write([]byte(origMsg))
	require.NoError(t, err)

	err = wc.Close()
	require.NoError(t, err)

	md, err := openpgp.ReadMessage(buf, el, nil, nil)
	require.NoError(t, err)

	ubBytes, err := ioutil.ReadAll(md.UnverifiedBody)
	require.NoError(t, err)
	require.Equal(t, origMsg, string(ubBytes))
}
