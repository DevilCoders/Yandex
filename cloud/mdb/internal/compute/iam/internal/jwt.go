package internal

import (
	"crypto/rsa"
	"time"

	"github.com/golang-jwt/jwt/v4"

	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// NOTE(skipor): by default, Go RSA PSS uses PSSSaltLengthAuto, which is not accepted by jwt.io and some python libraries.
// Should be removed after https://github.com/dgrijalva/jwt-go/issues/285 fix.
var jwtSigningMethodPS256WithSaltLengthEqualsHash = &jwt.SigningMethodRSAPSS{
	SigningMethodRSA: jwt.SigningMethodPS256.SigningMethodRSA,
	Options: &rsa.PSSOptions{
		SaltLength: rsa.PSSSaltLengthEqualsHash,
	},
}

// ServiceAccountToken retrieves IAM token for provided service account token
func JWTFromServiceAccount(serviceAccount iam.ServiceAccount) (string, error) {
	now := time.Now().Unix()
	claims := jwt.StandardClaims{
		Issuer:    serviceAccount.ID,
		Audience:  "https://iam.api.cloud.yandex.net/iam/v1/tokens",
		IssuedAt:  now,
		ExpiresAt: now + 3600,
	}
	token := jwt.NewWithClaims(jwtSigningMethodPS256WithSaltLengthEqualsHash, claims)
	token.Header["typ"] = "JWT"
	token.Header["alg"] = "PS256"
	token.Header["kid"] = serviceAccount.KeyID
	key, err := jwt.ParseRSAPrivateKeyFromPEM(serviceAccount.Token)
	if err != nil {
		return "", xerrors.Errorf("failed to parse private key for service account: %w", err)
	}
	jwt, err := token.SignedString(key)
	if err != nil {
		return "", xerrors.Errorf("failed to make JWT token: %w", err)
	}
	return jwt, nil
}
