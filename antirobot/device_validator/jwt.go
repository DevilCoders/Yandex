package main

import (
	"crypto/x509"
	"encoding/base64"
	"errors"
	"fmt"

	"github.com/golang-jwt/jwt/v4"
)

func CheckJwt(tokenStr string, validMethods []string, hostname string) (*jwt.Token, error) {
	parser := &jwt.Parser{
		UseJSONNumber: true,
		ValidMethods:  validMethods,
	}

	token, err := parser.Parse(tokenStr, makeJwtKeyFunc(hostname))
	if err != nil {
		return token, fmt.Errorf("failed to parse JWT: %w", err)
	}

	if !token.Valid {
		return token, errors.New("invalid JWT")
	}

	if err := token.Claims.Valid(); err != nil {
		return token, fmt.Errorf("invalid JWT claims: %w", err)
	}

	return token, nil
}

func makeJwtKeyFunc(hostname string) func(token *jwt.Token) (interface{}, error) {
	return func(token *jwt.Token) (interface{}, error) {
		certStrings, ok := token.Header["x5c"].([]interface{})
		if !ok {
			return nil, errors.New("missing certificates (x5c) in header")
		}

		if len(certStrings) < 1 {
			return nil, errors.New("empty certificate list in header")
		}

		mainCert, err := parseJwtCertificate(certStrings[0])
		if err != nil {
			return nil, fmt.Errorf("bad main certificate: %w", err)
		}

		intermediateCerts := x509.NewCertPool()

		for i := 1; i < len(certStrings); i += 1 {
			cert, err := parseJwtCertificate(certStrings[i])
			if err != nil {
				return nil, fmt.Errorf("bad certificate at index %d: %w", i, err)
			}

			intermediateCerts.AddCert(cert)
		}

		verifyOpts := x509.VerifyOptions{
			DNSName:       hostname,
			Intermediates: intermediateCerts,
		}
		if _, err := mainCert.Verify(verifyOpts); err != nil {
			return nil, fmt.Errorf("certificate verification failure: %w", err)
		}

		return mainCert.PublicKey, nil
	}
}

func parseJwtCertificate(certStringUntyped interface{}) (*x509.Certificate, error) {
	certString, ok := certStringUntyped.(string)
	if !ok {
		return nil, errors.New("invalid type in certificate list")
	}

	certBytes := make([]byte, base64.StdEncoding.DecodedLen(len(certString)))
	certBytesLen, err := base64.StdEncoding.Decode(certBytes, []byte(certString))
	if err != nil {
		return nil, fmt.Errorf("invalid base64 encoding")
	}

	cert, err := x509.ParseCertificate(certBytes[:certBytesLen])
	if err != nil {
		return nil, fmt.Errorf("cannot parse certificate: %w", err)
	}

	return cert, nil
}
