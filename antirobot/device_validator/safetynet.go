package main

import (
	"errors"

	"github.com/golang-jwt/jwt/v4"
)

func CheckSafetyNetAttestation(attestation string) (jwt.MapClaims, error) {
	token, err := CheckJwt(
		attestation,
		[]string{"RS256", "RS384", "RS512"},
		"attest.android.com",
	)

	// token may be non-nil even if err is also not nil.
	claims := jwt.MapClaims{}
	claimsOk := false
	if token != nil {
		claims, claimsOk = token.Claims.(jwt.MapClaims)
	}

	if err != nil {
		return claims, err
	}

	if !claimsOk {
		return jwt.MapClaims{}, errors.New("invalid claims type")
	}

	return claims, nil
}
