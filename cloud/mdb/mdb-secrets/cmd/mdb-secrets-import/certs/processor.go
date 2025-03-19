package certs

import (
	"context"
	"crypto/x509"
	"io/ioutil"
	"path/filepath"
	"strings"
	"time"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/mdb/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
)

const (
	ca = "InternalCA"

	extCrt = ".crt"
	extKey = ".key"
)

type Processor struct {
	Secrets secretsdb.Service
}

//ProcessLine accepts line that contains path to .crt and .key files
func (i *Processor) ProcessLine(line string) error {
	parts := strings.Split(line, " ")
	if len(parts) != 2 {
		return xerrors.New("certs parameter is not of 2 parts")
	}
	var crtFileName, keyFileName string

	for _, v := range parts {
		switch {
		case strings.HasSuffix(v, extCrt) && crtFileName == "":
			crtFileName = v
		case strings.HasSuffix(v, extKey) && keyFileName == "":
			keyFileName = v
		default:
			return xerrors.Errorf("unknown parameter %s", v)
		}
	}
	if crtFileName == "" {
		return xerrors.New("no .crt file in input")
	}
	if keyFileName == "" {
		return xerrors.New("no .key file in input")
	}
	var hostname string

	crtHostname := strings.TrimSuffix(filepath.Base(crtFileName), filepath.Ext(crtFileName))
	if crtHostname != strings.TrimSuffix(filepath.Base(keyFileName), filepath.Ext(keyFileName)) {
		return xerrors.New(".crt and .key have different names")
	}

	hostname = crtHostname

	//parse cert
	crtRaw, err := ioutil.ReadFile(crtFileName)
	if err != nil {
		return err
	}

	certs, _ := crypto.CertificatesFromPem(crtRaw)
	if len(certs) == 0 {
		return xerrors.New("no certificates in file")
	}

	//key
	keyRaw, err := ioutil.ReadFile(keyFileName)
	if err != nil {
		return err
	}

	// TODO: выяснить на ревью что тут должно быть
	return i.Secrets.InsertNewCert(context.Background(), hostname, ca, []string{}, secretsdb.CertUpdate{
		Key:        string(keyRaw),
		Crt:        string(crtRaw),
		Expiration: calcExpiration(certs),
	})
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
