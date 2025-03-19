package nacl

import (
	"crypto/rand"
	"io"

	"golang.org/x/crypto/nacl/box"

	"a.yandex-team.ru/library/go/core/xerrors"
)

const nonceLength = 24

// Box wraps crypto/nacl/box into an object
type Box struct {
	PeersPublicKey [keyLength]byte
	PrivateKey     [keyLength]byte
}

// Seal implements crypto/nacl/box Seal
func (b *Box) Seal(data []byte) ([]byte, error) {
	var nonce [nonceLength]byte
	if _, err := io.ReadFull(rand.Reader, nonce[:]); err != nil {
		return nil, xerrors.New("failed to generate nonce")
	}

	return box.Seal(nonce[:], data, &nonce, &b.PeersPublicKey, &b.PrivateKey), nil
}

// Open implements crypto/nacl/box Open
func (b *Box) Open(encrypted []byte) ([]byte, bool) {
	var nonce [nonceLength]byte
	copy(nonce[:], encrypted[:nonceLength])
	return box.Open(nil, encrypted[nonceLength:], &nonce, &b.PeersPublicKey, &b.PrivateKey)
}
