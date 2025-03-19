package cloudjwt

import (
	"crypto/rsa"
	"errors"
	"time"

	"github.com/golang-jwt/jwt/v4"
)

var ErrNoPrivateKey = errors.New("no private key for JWT")

func (t *Authenticator) getJWT() (string, error) {
	issuedAt := time.Now()
	ps256WithSaltLengthEqualsHash := &jwt.SigningMethodRSAPSS{
		SigningMethodRSA: jwt.SigningMethodPS256.SigningMethodRSA,
		Options: &rsa.PSSOptions{
			SaltLength: rsa.PSSSaltLengthEqualsHash,
		},
	}
	jwtToken := jwt.NewWithClaims(ps256WithSaltLengthEqualsHash, jwt.StandardClaims{
		Issuer:    t.accountID,
		IssuedAt:  issuedAt.Unix(),
		ExpiresAt: issuedAt.Add(time.Hour).Unix(),
		Audience:  t.audience,
	})
	jwtToken.Header["kid"] = t.keyID

	if t.privateKey == nil {
		return "", ErrNoPrivateKey
	}

	return jwtToken.SignedString(t.privateKey)
}
