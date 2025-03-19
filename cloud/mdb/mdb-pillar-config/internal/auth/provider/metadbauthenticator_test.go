package provider

import (
	"context"
	"encoding/base64"
	"fmt"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"
	"golang.org/x/crypto/argon2"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/metadb/mocks"
)

func generatePasswordHashWithParams(password string, version, keyLen, time, memory uint32, threads uint8) string {
	salt := []byte("some_random_salt")
	saltEncoded := base64.RawStdEncoding.EncodeToString(salt)
	passwordHash := base64.RawStdEncoding.EncodeToString(argon2.Key([]byte(password), salt, time, memory, threads, keyLen))
	return fmt.Sprintf("$argoni$v=%d$m=%d,t=%d,p=%d$%s$%s", version, memory, time, threads, saltEncoded, passwordHash)
}

func TestGeneratePasswordHelper(t *testing.T) {
	t.Run("Default params password generation", func(t *testing.T) {
		require.Equal(t,
			"$argoni$v=19$m=512,t=2,p=2$c29tZV9yYW5kb21fc2FsdA$yBV87WUZJgh1pAqxoVH5hZWZy2QhzPpQOMqd44AD7Bk",
			generatePasswordHash("password"),
		)
	})

	t.Run("Another params password generation", func(t *testing.T) {
		require.Equal(t,
			"$argoni$v=19$m=512,t=4,p=4$c29tZV9yYW5kb21fc2FsdA$wZhwbqCUsGGE0/Rbej5pd6lz/d85qg",
			generatePasswordHashWithParams("password", argon2.Version, 22, 4, 512, 4),
		)
	})
}

func generatePasswordHash(password string) string {
	return generatePasswordHashWithParams(password, 19, 32, 2, 512, 2)
}

func TestPillarConfigAuth(t *testing.T) {
	ctrl := gomock.NewController(t)

	t.Run("No auth data", func(t *testing.T) {
		authenticator := MetaDBAuthenticator{authDataProvider: mocks.NewMockMetaDB(ctrl)}
		err := authenticator.Authenticate(context.Background(), nil)
		require.Error(t, err)
	})

	t.Run("Invalid access id", func(t *testing.T) {
		metaDB := mocks.NewMockMetaDB(ctrl)
		metaDB.EXPECT().ConfigHostAuth(gomock.Any(), gomock.Any()).Return(metadb.ConfigHostAuthInfo{}, semerr.Authentication(""))
		authenticator := MetaDBAuthenticator{authDataProvider: metaDB}
		err := authenticator.Authenticate(auth.WithAuth(context.Background(), "invalid-id", "secret"), nil)
		require.Error(t, err)
	})

	t.Run("Invalid access type", func(t *testing.T) {
		metaDB := mocks.NewMockMetaDB(ctrl)
		metaDB.EXPECT().ConfigHostAuth(gomock.Any(), gomock.Any()).Return(metadb.ConfigHostAuthInfo{AccessType: "dbaas-worker"}, nil)
		authenticator := MetaDBAuthenticator{authDataProvider: metaDB}
		err := authenticator.Authenticate(auth.WithAuth(context.Background(), "access-id", "secret"), []string{"default"})
		require.Error(t, err)
	})

	t.Run("Invalid access secret in metadb", func(t *testing.T) {
		metaDB := mocks.NewMockMetaDB(ctrl)
		metaDB.EXPECT().ConfigHostAuth(gomock.Any(), gomock.Any()).Return(metadb.ConfigHostAuthInfo{AccessType: "default"}, nil)
		authenticator := MetaDBAuthenticator{authDataProvider: metaDB}
		err := authenticator.Authenticate(auth.WithAuth(context.Background(), "access-id", "secret"), []string{"default"})
		require.Error(t, err)
	})

	t.Run("Invalid access secret", func(t *testing.T) {
		metaDB := mocks.NewMockMetaDB(ctrl)
		metaDB.EXPECT().ConfigHostAuth(gomock.Any(), gomock.Any()).Return(metadb.ConfigHostAuthInfo{AccessType: "default", AccessSecret: "$argon2i$v=19$m=512,t=2,p=2$aaaaaa$aaaaaaa"}, nil)
		authenticator := MetaDBAuthenticator{authDataProvider: metaDB}
		err := authenticator.Authenticate(auth.WithAuth(context.Background(), "access-id", "secret"), []string{"default"})
		require.Error(t, err)
	})

	t.Run("auth success", func(t *testing.T) {
		password := "strong_password"
		passwordHash := generatePasswordHash(password)
		metaDB := mocks.NewMockMetaDB(ctrl)
		metaDB.EXPECT().ConfigHostAuth(gomock.Any(), gomock.Any()).Return(metadb.ConfigHostAuthInfo{AccessType: "default", AccessSecret: passwordHash}, nil)
		authenticator := MetaDBAuthenticator{authDataProvider: metaDB}
		require.NoError(t, authenticator.Authenticate(auth.WithAuth(context.Background(), "access-id", password), []string{"default"}))
	})
}

func TestPasswordVerification(t *testing.T) {
	password := "strong_password"

	t.Run("Password correct", func(t *testing.T) {
		ok, err := verifySecret(password, generatePasswordHash(password))
		require.NoError(t, err)
		require.Equal(t, true, ok)
	})

	t.Run("Different parameters works", func(t *testing.T) {
		ok, err := verifySecret(password, generatePasswordHashWithParams(password, 19, 22, 4, 1024, 4))
		require.NoError(t, err)
		require.Equal(t, true, ok)
	})

	t.Run("Password incorrect", func(t *testing.T) {
		ok, err := verifySecret("invalid_password", generatePasswordHash(password))
		require.NoError(t, err)
		require.Equal(t, false, ok)
	})

	t.Run("Invalid version", func(t *testing.T) {
		ok, err := verifySecret("invalid_password", generatePasswordHashWithParams(password, 20, 32, 2, 512, 2))
		require.Error(t, err)
		require.Equal(t, false, ok)
	})

	t.Run("Invalid parameters", func(t *testing.T) {
		ok, err := verifySecret(password, "$argoni$v=19$m=5e12,t=2,p=2$aaaaaaa$aaaaaaa")
		require.Error(t, err)
		require.Equal(t, false, ok)
	})
}
