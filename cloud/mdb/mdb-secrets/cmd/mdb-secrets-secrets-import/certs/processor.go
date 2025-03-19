package certs

import (
	"context"
	"encoding/base64"

	"a.yandex-team.ru/cloud/mdb/internal/nacl"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Processor struct {
	SrcSecrets          secretsdb.Service
	DstSecrets          secretsdb.Service
	L                   log.Logger
	SrcSecretsPublicKey [32]byte
	SrcSaltPrivateKey   [32]byte
}

func (i *Processor) decryptData(msg string) ([]byte, error) {
	encryptedData, err := base64.URLEncoding.DecodeString(msg)
	if err != nil {
		return nil, xerrors.Errorf("decode: %w. Verify that is URL-encoded base64", err)
	}
	box := nacl.Box{
		PeersPublicKey: i.SrcSecretsPublicKey,
		PrivateKey:     i.SrcSaltPrivateKey,
	}
	decrypted, ok := box.Open(encryptedData)
	if !ok {
		return nil, xerrors.New("decryption failed. Verify your keys")
	}
	return decrypted, nil
}

func (i *Processor) CopyCrt(ctx context.Context, hostname string, dryRun bool) error {
	cert, err := i.SrcSecrets.GetCert(ctx, hostname)
	if xerrors.Is(err, secretsdb.ErrCertNotFound) {
		i.L.Infof("%s: not found in src", hostname)
		return nil
	}
	if err != nil {
		return err
	}

	if cert.Key.Data == "" {
		i.L.Infof("%s: dummy", hostname)
		return nil
	}

	keyRaw, err := i.decryptData(cert.Key.Data)
	if err != nil {
		return xerrors.Errorf("decrypt key: %w", err)
	}

	_, err = i.DstSecrets.GetCert(ctx, hostname)
	if xerrors.Is(err, secretsdb.ErrCertNotFound) {
		if dryRun {
			i.L.Infof("%s: would have inserted. AltNames: %s, Expiration: %s", hostname, cert.AltNames, cert.Expiration)
			return nil
		}
		err := i.DstSecrets.InsertNewCert(ctx, hostname, cert.CA, cert.AltNames, secretsdb.CertUpdate{
			Key:        string(keyRaw),
			Crt:        cert.Crt,
			Expiration: cert.Expiration,
			AltNames:   cert.AltNames,
		})
		if err == nil {
			i.L.Infof("%s: put success", hostname)
		}
		return err
	}
	if err != nil {
		return err
	}

	if dryRun {
		i.L.Infof("%s: would have updated. AltNames: %s, Expiration: %s", hostname, cert.AltNames, cert.Expiration)
		return nil
	}
	i.L.Infof("%s: found old cert", hostname)
	_, err = i.DstSecrets.UpdateCert(ctx, hostname, cert.CA, func(ctx context.Context) (*secretsdb.CertUpdate, error) {
		return &secretsdb.CertUpdate{
			Key:        string(keyRaw),
			Crt:        cert.Crt,
			Expiration: cert.Expiration,
			AltNames:   cert.AltNames,
		}, nil
	}, func(cert *secretsdb.Cert) bool {
		return true
	})
	if err == nil {
		i.L.Infof("%s: updated", hostname)
	}
	return err
}

func (i *Processor) CopyGpg(ctx context.Context, cid string, dryRun bool) error {
	gpg, err := i.SrcSecrets.GetGpg(ctx, cid)
	if xerrors.Is(err, secretsdb.ErrKeyNotFound) {
		i.L.Infof("%s not found in src", cid)
		return nil
	}
	if err != nil {
		return err
	}
	rawKey, err := i.decryptData(gpg.Data)
	if err != nil {
		return xerrors.Errorf("decrypt key: %w", err)
	}
	// check if there already is a key
	if _, err := i.DstSecrets.GetGpg(ctx, cid); err != nil {
		if xerrors.Is(err, secretsdb.ErrKeyNotFound) {
			if dryRun {
				i.L.Infof("%s: would have put key", cid)
				return nil
			}
			_, err = i.DstSecrets.PutGpg(ctx, cid, string(rawKey))
			if err != nil {
				return xerrors.Errorf("put gpg: %w", err)
			}
			i.L.Infof("%s put success", cid)
			return nil
		} else {
			return err
		}
	}
	if dryRun {
		i.L.Infof("%s: would have deleted old key and put new", cid)
		return nil
	}
	i.L.Infof("%s old gpg found", cid)
	if err := i.DstSecrets.DeleteGpg(ctx, cid); err != nil {
		return xerrors.Errorf("delete gpg: %w", err)
	}
	i.L.Infof("%s old gpg deleted", cid)
	if _, err := i.DstSecrets.PutGpg(ctx, cid, string(rawKey)); err != nil {
		return xerrors.Errorf("put gpg: %w", err)
	}
	i.L.Infof("%s put success", cid)
	return nil
}
