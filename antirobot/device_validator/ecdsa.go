package main

import (
	"crypto/ecdsa"
	"crypto/x509"
	"encoding/pem"
	"errors"
	"fmt"
)

func DecodeECDSAPrivateKey(b []byte) (*ecdsa.PrivateKey, error) {
	pemBlock, _ := pem.Decode(b)
	if pemBlock == nil {
		return nil, errors.New("failed to decode PEM")
	}

	privateKeyUntyped, err := x509.ParsePKCS8PrivateKey(pemBlock.Bytes)
	if err != nil {
		return nil, fmt.Errorf("failed to decode PKCS8: %w", err)
	}

	privateKey, ok := privateKeyUntyped.(*ecdsa.PrivateKey)
	if !ok {
		return nil, fmt.Errorf("unexpected key type: %T", privateKeyUntyped)
	}

	return privateKey, nil
}
