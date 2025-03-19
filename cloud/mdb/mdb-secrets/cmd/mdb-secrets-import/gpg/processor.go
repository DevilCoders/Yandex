package gpg

import (
	"bytes"
	"context"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strings"

	"golang.org/x/crypto/openpgp"
	"golang.org/x/crypto/openpgp/armor"

	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Processor struct {
	Secrets secretsdb.Service
}

func (p *Processor) ProcessLine(filename string) error {
	cid, err := extractCid(filename)
	if err != nil {
		return err
	}

	armoredKey, err := getKey(filename)
	if err != nil {
		return err
	}
	_, err = p.Secrets.GetGpg(context.Background(), cid)
	switch {
	case xerrors.Is(err, secretsdb.ErrKeyNotFound):
		_, err = p.Secrets.PutGpg(context.Background(), cid, armoredKey)
		if err != nil {
			return err
		}
		return nil // success
	default:
		return err
	}
}

func getKey(filename string) (string, error) {
	el, err := readEntities(filename)
	if err != nil {
		return "", err
	}

	buf := &bytes.Buffer{}

	if len(el) != 1 {
		return "", fmt.Errorf("keyring contains (%d) entities, but should contain one", len(el))
	}
	if el[0].PrivateKey == nil {
		return "", fmt.Errorf("keyring doesn't contain any private keys")
	}

	err = writePrivateKey(el[0], buf)
	if err != nil {
		return "", err
	}

	return buf.String(), nil
}

func extractCid(filename string) (string, error) {
	file := filepath.Base(filename)
	ext := filepath.Ext(filename)
	cid := strings.TrimSuffix(file, ext)
	if cid == "" {
		return cid, fmt.Errorf("file should be named with cluster id")
	}
	return cid, nil
}

func writePrivateKey(entity *openpgp.Entity, writer io.Writer) error {
	wc, err := armor.Encode(writer, openpgp.PrivateKeyType, nil)
	if err != nil {
		return err
	}
	defer func() { _ = wc.Close() }()

	return entity.SerializePrivate(wc, nil)
}

func readEntities(keyringFile string) (openpgp.EntityList, error) {
	file, err := os.Open(keyringFile)
	if err != nil {
		return nil, err
	}
	return openpgp.ReadKeyRing(file)
}
