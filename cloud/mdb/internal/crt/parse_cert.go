package crt

import (
	"bytes"
	"crypto/x509"
	"encoding/pem"
	"fmt"
	"time"

	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	certBlockType = "CERTIFICATE"
	keyBlockType1 = "PRIVATE KEY"
	keyBlockType2 = "RSA PRIVATE KEY"
)

var CertParseErr = xerrors.NewSentinel("could not parse certificate")

func ParseCert(pemData []byte) (*Cert, error) {
	var certBlocks []byte
	certBuf := &bytes.Buffer{}
	keyBuf := &bytes.Buffer{}

	block, rest := pem.Decode(pemData)
	for block != nil {
		switch block.Type {
		case certBlockType:
			certBlocks = append(certBlocks, block.Bytes...)
			err := pem.Encode(certBuf, block)
			if err != nil {
				return nil, CertParseErr.Wrap(err)
			}
		case keyBlockType1, keyBlockType2:
			err := pem.Encode(keyBuf, block)
			if err != nil {
				return nil, CertParseErr.Wrap(err)
			}
		}
		if len(rest) == 0 {
			break
		}
		block, rest = pem.Decode(rest)
	}
	certs, err := x509.ParseCertificates(certBlocks)
	if err != nil {
		return nil, err
	}
	if len(certs) == 0 {
		return nil, CertParseErr.Wrap(fmt.Errorf("no certs in certificator response"))
	}

	expire := calcExpiration(certs)

	if keyBuf.Len() == 0 {
		return nil, CertParseErr.Wrap(fmt.Errorf("could not get key"))
	}
	if certBuf.Len() == 0 {
		return nil, CertParseErr.Wrap(fmt.Errorf("could not get cert"))
	}

	return &Cert{
		CertPem:    certBuf.Bytes(),
		KeyPem:     keyBuf.Bytes(),
		Expiration: expire,
	}, nil
}

func calcExpiration(certs []*x509.Certificate) time.Time {
	expire := certs[0].NotAfter
	for _, c := range certs {
		// we need earliest expiration date
		if c.NotAfter.Before(expire) {
			expire = c.NotAfter
		}
	}
	return expire
}
