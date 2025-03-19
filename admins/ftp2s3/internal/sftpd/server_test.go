package sftpd

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"net"
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"
	"golang.org/x/crypto/ssh"

	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func init() {
	log = &nop.Logger{}
}

var (
	user1      = "user1"
	pubKeyStr1 = "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAAAgQDNbTh9MAXME3NsM445R2fz4BK8" +
		"5N98MTH4NyapS/igmGbDLJWzbxlvvmyjbut1KGqpHxGgyWjUekeCBhK+AnHLrL9tVcOlPm" +
		"Iozn0D8cZtnMxNhrlkEIJ7tDjmd0HAmfAPs4FKk7IftTm8L1uTam001XMpypDOGNONHG22" +
		"E4piCQ== ignored comments here"
	user2      = "user2"
	pubKeyStr2 = "ecdsa-sha2-nistp256 AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAy" +
		"NTYAAABBBG+Ut9izbqZqBpYnw/tdj9kbLuX3a0JdQm1ubf5Np76eceXYntTgEUdmyPAYD4" +
		"7I1zUHv3YHl15mV6XdTicEQBA= ignored comments here"
	pubKeyStr3 = "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIObVNzIcRIKdGvfL570RBAWGGrDW3kcY1SUAC2zNJ3CA"
)

type pubKeys map[string]string

func TestLoadAuthorizedKyes(t *testing.T) {
	onlyKeyPubKey2 := strings.SplitN(pubKeyStr1, " ", 3)[1]
	cases := []struct {
		pubKeys map[string]string
		failed  error
	}{
		{pubKeys{user1: pubKeyStr1}, nil},
		{pubKeys{user2: onlyKeyPubKey2}, ErrInvalidPublicKey},
		{pubKeys{user1: pubKeyStr1, user2: pubKeyStr2}, nil},
		{pubKeys{user2: "ops" + onlyKeyPubKey2}, ErrInvalidPublicKey},
	}

	for idx, c := range cases {
		t.Logf("Case #%d %v", idx, c)
		pubKeys, err := json.Marshal(c.pubKeys)
		if err != nil {
			assert.FailNow(t, "Unexpected err", "%v", err)
		}
		if idx == 0 {
			_, err = loadAuthorizedKeys(bytes.NewBuffer(append(pubKeys, byte(':'))))
			assert.Error(t, err, "expect to fail on invalid json")
		}
		users, err := loadAuthorizedKeys(bytes.NewBuffer(pubKeys))
		if !assert.True(t, xerrors.Is(err, c.failed)) {
			t.Logf("Unexpected err: %v", err)
		}
		for user, pubKey := range users {
			onlyKey := strings.SplitN(c.pubKeys[user], " ", 3)[1]
			assert.Equal(t, base64.StdEncoding.EncodeToString([]byte(pubKey)), onlyKey)
		}
	}
}

type mockConnMetadata struct {
	user string
}

func (m *mockConnMetadata) User() string          { return m.user }
func (m *mockConnMetadata) SessionID() []byte     { return nil }
func (m *mockConnMetadata) ClientVersion() []byte { return nil }
func (m *mockConnMetadata) ServerVersion() []byte { return nil }
func (m *mockConnMetadata) RemoteAddr() net.Addr  { return nil }
func (m *mockConnMetadata) LocalAddr() net.Addr   { return nil }

func TestPublicKeyCallback(t *testing.T) {
	keysJSON, err := json.Marshal(pubKeys{user1: pubKeyStr1, user2: pubKeyStr2})
	if err != nil {
		assert.FailNow(t, "Unexpected err", "%v", err)
	}
	users, err := loadAuthorizedKeys(bytes.NewBuffer(keysJSON))
	if err != nil {
		assert.FailNow(t, "Unexpected err", "%v", err)
	}
	config := createServerConfig(users, solomon.NewRegistry(nil))
	pubkey1, err1 := ssh.ParsePublicKey([]byte(users[user1]))
	pubkey2, err2 := ssh.ParsePublicKey([]byte(users[user2]))
	pubkey3, _, _, _, err3 := ssh.ParseAuthorizedKey([]byte(pubKeyStr3))
	if err1 != nil || err2 != nil || err3 != nil {
		assert.FailNow(t, "Unexpected err", "%v | %v | %v", err1, err2, err3)
	}

	_, err = config.PublicKeyCallback(&mockConnMetadata{user: user1}, pubkey1)
	assert.NoError(t, err)
	_, err = config.PublicKeyCallback(&mockConnMetadata{user: user2}, pubkey2)
	assert.NoError(t, err)

	_, err = config.PublicKeyCallback(&mockConnMetadata{user: user2}, pubkey1)
	assert.True(t, xerrors.Is(err, ErrInvalidPublicKey))

	_, err = config.PublicKeyCallback(&mockConnMetadata{user: user1}, pubkey2)
	assert.True(t, xerrors.Is(err, ErrInvalidPublicKey))

	_, err = config.PublicKeyCallback(&mockConnMetadata{user: user1}, pubkey3)
	assert.True(t, xerrors.Is(err, ErrInvalidPublicKey))
	_, err = config.PublicKeyCallback(&mockConnMetadata{user: "user3"}, pubkey3)
	assert.True(t, xerrors.Is(err, ErrUnknownUser))
}
