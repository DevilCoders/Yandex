package ftpd

import (
	"bytes"
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"
	"golang.org/x/crypto/bcrypt"

	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	goodPass = "Sup3Rp@ss"
	badPass  = "pass"
)

func init() {
	log = &nop.Logger{}
}

func getGoodAndBashPwdHashes(t *testing.T) (string, string) {
	hashGoodPass, err := bcrypt.GenerateFromPassword([]byte(goodPass), bcrypt.DefaultCost)
	stringHashGoodPass := string(hashGoodPass)
	if err != nil {
		t.Log("Unexptected error", err)
		t.FailNow()
	}
	hashBadPass, err := bcrypt.GenerateFromPassword([]byte(badPass), 4)
	stringHashBadPass := string(hashBadPass)
	if err != nil {
		t.Log("Unexptected error", err)
		t.FailNow()
	}

	return stringHashGoodPass, stringHashBadPass
}

func TestGetUsers(t *testing.T) {
	goodHash, badHash := getGoodAndBashPwdHashes(t)

	cases := []struct {
		users  []User
		failed error
	}{
		{users: []User{{"u", goodHash}}, failed: ErrInvalidName},
		{users: []User{{"-u", goodHash}}, failed: ErrInvalidName},
		{users: []User{{"u1", "invalid hash"}}, failed: ErrInvalidPassword},
		{users: []User{{"u1", badHash}}, failed: ErrPasswordHashTooWeak},
		{users: []User{{"u1", goodHash}}, failed: nil},
		{users: []User{{"u1", goodHash}, {"u1", goodHash}}, failed: ErrDuplicateUser},
		{users: []User{{"u1", goodHash}, {"u2", goodHash}}, failed: nil},
	}
	for idx, c := range cases {
		t.Logf("Case #%d %v", idx, c)
		usersBytes, err := json.Marshal(c.users)
		if !assert.NoError(t, err) {
			t.FailNow()
		}
		server := FtpdService{}
		err = server.getUsers(bytes.NewBuffer(usersBytes))
		if c.failed == nil && err != nil || !assert.True(t, xerrors.Is(err, c.failed)) {
			t.Logf("Unexpected err: %v", err)
		}
	}
}

func TestAuthUser(t *testing.T) {
	goodHash, _ := getGoodAndBashPwdHashes(t)

	users := []User{
		{"u1", goodHash},
		{"u2", goodHash},
	}
	usersBytes, err := json.Marshal(users)
	if !assert.NoError(t, err) {
		t.FailNow()
	}

	server := FtpdService{metrics: solomon.NewRegistry(nil)}
	err = server.getUsers(bytes.NewBuffer(usersBytes))
	if err != nil {
		t.Log("Unexpected err: ", err)
		t.FailNow()
	}

	_, err = server.AuthUser(nil, users[0].Name, goodPass)
	assert.NoError(t, err)

	_, err = server.AuthUser(nil, users[0].Name, goodPass+"ops")
	assert.True(t, xerrors.Is(err, ErrInvalidPassword))

	_, err = server.AuthUser(nil, users[1].Name, goodPass)
	assert.NoError(t, err)

	_, err = server.AuthUser(nil, users[1].Name, badPass)
	assert.True(t, xerrors.Is(err, ErrInvalidPassword))

	_, err = server.AuthUser(nil, "u3", badPass)
	assert.True(t, xerrors.Is(err, ErrInvalidLogin))
}
