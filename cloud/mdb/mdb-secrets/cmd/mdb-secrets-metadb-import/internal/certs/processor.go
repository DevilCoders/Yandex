package certs

import (
	"context"
	"crypto/x509"
	"encoding/base64"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/internal/nacl"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

type Cert struct {
	Hostname string
	Crt      []byte
	Key      []byte
}

type Processor struct {
	Secrets              secretsdb.Service
	L                    log.Logger
	InternalAPIPublicKey [32]byte
	SaltPrivateKey       [32]byte
}

func (i *Processor) decryptData(msg string) ([]byte, error) {
	encryptedData, err := base64.URLEncoding.DecodeString(msg)
	if err != nil {
		return nil, xerrors.Errorf("decode: %w. Verify that is URL-encoded base64", err)
	}
	box := nacl.Box{
		PeersPublicKey: i.InternalAPIPublicKey,
		PrivateKey:     i.SaltPrivateKey,
	}
	decrypted, ok := box.Open(encryptedData)
	if !ok {
		return nil, xerrors.New("decryption failed. Verify your keys")
	}
	return decrypted, nil
}

func (i *Processor) Process(ctx context.Context, hostname, ca, encryptedCrt, encryptedKey string, dryRun bool) error {
	crtRaw, err := i.decryptData(encryptedCrt)
	if err != nil {
		return xerrors.Errorf("decrypt crt: %w", err)
	}
	keyRaw, err := i.decryptData(encryptedKey)
	if err != nil {
		return xerrors.Errorf("decrypt key: %w", err)
	}
	certs, err := crypto.CertificatesFromPem(crtRaw)
	if err != nil {
		return xerrors.Errorf("read certificates from pem: %w", err)
	}
	if len(certs) == 0 {
		return xerrors.New("no certificates in file")
	}
	altNames := calcAltNames(certs)
	expiration := calcExpiration(certs)

	if dryRun {
		i.L.Infof("Parsed %q certificates in dry run. AltNames: %s, Expiration: %s", hostname, altNames, expiration)
		return nil
	}
	_, err = i.Secrets.GetCert(ctx, hostname)
	if xerrors.Is(err, secretsdb.ErrCertNotFound) {
		return i.Secrets.InsertNewCert(ctx, hostname, ca, altNames, secretsdb.CertUpdate{
			Key:        string(keyRaw),
			Crt:        string(crtRaw),
			Expiration: expiration,
			AltNames:   altNames,
		})
	}
	if err != nil {
		return err
	}

	_, err = i.Secrets.UpdateCert(ctx, hostname, ca, func(ctx context.Context) (*secretsdb.CertUpdate, error) {
		return &secretsdb.CertUpdate{
			Key:        string(keyRaw),
			Crt:        string(crtRaw),
			Expiration: expiration,
			AltNames:   altNames,
		}, nil
	}, func(cert *secretsdb.Cert) bool {
		return true
	})
	if err == nil {
		i.L.Infof("%q updated", hostname)
	}
	return err
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

func calcAltNames(certs []*x509.Certificate) []string {
	var names []string
	for _, c := range certs {
		names = append(names, c.DNSNames...)
	}
	return slices.DedupStrings(names)
}
