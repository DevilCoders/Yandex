package gpg

import (
	"bytes"
	"crypto"

	"golang.org/x/crypto/openpgp"
	"golang.org/x/crypto/openpgp/armor"
	"golang.org/x/crypto/openpgp/packet"
)

const email = "mdb-admin@yandex-team.ru"

// GenerateGPG generates new pgp key and returns private key in OpenPGP armor
func GenerateGPG(cid string) (string, error) {
	buf := &bytes.Buffer{}
	wc, err := armor.Encode(buf, openpgp.PrivateKeyType, nil)
	if err != nil {
		return "", err
	}
	defer func() { _ = wc.Close() }()

	// creates new key with self-signed identity
	e, err := openpgp.NewEntity(cid, cid, email, &packet.Config{
		RSABits:       4096,
		DefaultHash:   crypto.SHA256,
		DefaultCipher: packet.CipherAES128,
	})
	if err != nil {
		return "", err
	}

	if err = e.SerializePrivate(wc, nil); err != nil {
		return "", err
	}
	// Close() closes armored block with appropriate ending
	err = wc.Close()

	return buf.String(), err
}
