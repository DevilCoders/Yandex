package letsencrypt

import (
	"bytes"
	"context"
	"crypto/rand"
	"crypto/rsa"
	"crypto/x509"
	"encoding/pem"

	"golang.org/x/crypto/acme"

	"a.yandex-team.ru/cloud/mdb/internal/crt"
	"a.yandex-team.ru/cloud/mdb/internal/crypto"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *LetsEncryptClient) IssueCert(ctx context.Context, target string, altNames []string, caName string, certType crt.CertificateType) (*crt.Cert, error) {
	if err := c.register(ctx); err != nil {
		return nil, xerrors.Errorf("register: %w", err)
	}

	authz := make([]acme.AuthzID, 0, len(altNames)+1)
	authz = append(authz, acme.AuthzID{Type: "dns", Value: target})
	for _, altName := range altNames {
		authz = append(authz, acme.AuthzID{Type: "dns", Value: altName})
	}

	order, err := c.acmeClient.AuthorizeOrder(ctx, authz)
	if err != nil {
		return nil, xerrors.Errorf("AuthorizeOrder: %w", err)
	}
	ctxlog.Debug(ctx, c.l, "created order", log.Any("order", order))

	if err := c.authorizeOrder(ctx, order); err != nil {
		return nil, xerrors.Errorf("authorizeOrder: %w", err)
	}
	ctxlog.Debug(ctx, c.l, "authorized order")

	privateKey, err := crypto.GeneratePrivateKey()
	if err != nil {
		return nil, err
	}

	DNSNames := make([]string, 0, len(altNames)+1)
	DNSNames = append(DNSNames, target)
	DNSNames = append(DNSNames, altNames...)
	ctxlog.Debug(ctx, c.l, "create csr for DNSNames", log.Array("DNSNames", DNSNames))
	csr, err := x509.CreateCertificateRequest(rand.Reader, &x509.CertificateRequest{DNSNames: DNSNames}, (*rsa.PrivateKey)(privateKey))
	if err != nil {
		return nil, err
	}
	ctxlog.Debug(ctx, c.l, "created csr", log.Binary("csr", csr))

	pemData := &bytes.Buffer{}
	keyRaw, err := privateKey.EncodeToPEM()
	if err != nil {
		return nil, err
	}
	if _, err := pemData.Write(keyRaw); err != nil {
		return nil, err
	}

	certsRaw, certURL, err := c.acmeClient.CreateOrderCert(context.Background(), order.FinalizeURL, csr, true)
	if err != nil {
		return nil, xerrors.Errorf("CreateOrderCert: %w", err)
	}

	for _, certBlock := range certsRaw {
		if err := pem.Encode(pemData, &pem.Block{Bytes: certBlock, Type: "CERTIFICATE"}); err != nil {
			return nil, err
		}
	}

	result, err := crt.ParseCert(pemData.Bytes())
	if err != nil {
		return nil, err
	}

	if err := c.storage.SetCertificateURL(ctx, target, certURL); err != nil {
		return nil, xerrors.Errorf("SetCertificateURL: %w", err)
	}

	return result, nil
}

func (c *LetsEncryptClient) authorizeOrder(ctx context.Context, order *acme.Order) error {
	for _, authzURL := range order.AuthzURLs {
		if err := c.authorizeURL(ctx, authzURL); err != nil {
			return xerrors.Errorf("authorizeURL %q: %w", authzURL, err)
		}
	}

	return nil
}

func (c *LetsEncryptClient) authorizeURL(ctx context.Context, authzURL string) error {
	auth, err := c.acmeClient.GetAuthorization(ctx, authzURL)
	if err != nil {
		return xerrors.Errorf("GetAuthorization: %w", err)
	}
	ctxlog.Debug(ctx, c.l, "got auth", log.String("authzURL", authzURL), log.Any("auth", auth))

	if auth.Status == acme.StatusValid {
		return nil
	}

	var chal *acme.Challenge
	for _, challenge := range auth.Challenges {
		if challenge.Type == c.challengeProvider.ChallengeType() {
			chal = challenge
			break
		}
	}
	if chal == nil {
		return xerrors.Errorf("%q challenge not found", c.challengeProvider.ChallengeType())
	}
	ctxlog.Debug(ctx, c.l, "got challenge", log.Any("challenge", chal))

	if err := c.challengeProvider.Prepare(ctx, c.acmeClient, *auth, *chal); err != nil {
		return xerrors.Errorf("challengeProvider.Prepare: %w", err)
	}

	defer func() {
		_ = c.challengeProvider.Cleanup(ctx, c.acmeClient, *auth, *chal)
	}()

	if _, err = c.acmeClient.Accept(ctx, chal); err != nil {
		return xerrors.Errorf("acmeClient.Accept: %w", err)
	}

	waitedAuth, err := c.acmeClient.WaitAuthorization(ctx, auth.URI)
	if err != nil {
		// returned auth here could be nil
		// and Cleanup function will panic with nil auth
		return xerrors.Errorf("acmeClient.WaitAuthorization: %w", err)
	}
	auth = waitedAuth

	if auth.Status != acme.StatusValid {
		return xerrors.Errorf("authorization failed %+v", *auth)
	}
	ctxlog.Debug(ctx, c.l, "authorized", log.Any("authID", auth.Identifier))

	return nil
}
