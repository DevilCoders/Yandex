package provider

import (
	"context"
	"crypto/subtle"
	"encoding/base64"
	"fmt"
	"strings"

	"golang.org/x/crypto/argon2"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/metadb"
)

type AuthDataProvider interface {
	ConfigHostAuth(ctx context.Context, accessID string) (metadb.ConfigHostAuthInfo, error)
}

type MetaDBAuthenticator struct {
	authDataProvider AuthDataProvider
}

var _ auth.Authenticator = &MetaDBAuthenticator{}

func NewMetaDBAuthenticator(authDataProvider AuthDataProvider) MetaDBAuthenticator {
	return MetaDBAuthenticator{
		authDataProvider: authDataProvider,
	}
}

func (a MetaDBAuthenticator) Authenticate(ctx context.Context, configTypes []string) error {
	id, secret, ok := auth.AuthDataFromContext(ctx)
	if !ok {
		return semerr.Authentication("missing auth data")
	}

	configAuth, err := a.authDataProvider.ConfigHostAuth(ctx, id)
	if err != nil {
		return err
	}

	for _, authType := range configTypes {
		if authType == configAuth.AccessType {
			if valid, err := verifySecret(secret, configAuth.AccessSecret); err != nil || !valid {
				return semerr.Authenticationf("invalid access secret, %q", err)
			}

			return nil
		}
	}

	return semerr.Authentication("access type mismatch")
}

func verifySecret(plain, hash string) (bool, error) {
	hashParts := strings.Split(hash, "$")
	if len(hashParts) != 6 {
		return false, semerr.Internal("failed to verify secret")
	}

	var version uint32
	_, err := fmt.Sscanf(hashParts[2], "v=%d", &version)
	if err != nil {
		return false, err
	}
	if version != argon2.Version {
		return false, semerr.Internal("failed to verify secret")
	}

	var memory, time uint32
	var threads uint8

	_, err = fmt.Sscanf(hashParts[3], "m=%d,t=%d,p=%d", &memory, &time, &threads)
	if err != nil {
		return false, err
	}

	salt, err := base64.RawStdEncoding.DecodeString(hashParts[4])
	if err != nil {
		return false, err
	}

	decodedHash, err := base64.RawStdEncoding.DecodeString(hashParts[5])
	if err != nil {
		return false, err
	}

	hashToCompare := argon2.Key([]byte(plain), salt, time, memory, threads, uint32(len(decodedHash)))

	return subtle.ConstantTimeCompare(decodedHash, hashToCompare) == 1, nil
}
