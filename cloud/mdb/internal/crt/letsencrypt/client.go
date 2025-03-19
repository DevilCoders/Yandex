package letsencrypt

import (
	"context"
	"crypto/rsa"
	"io/ioutil"
	"sync"

	"golang.org/x/crypto/acme"

	"a.yandex-team.ru/cloud/mdb/internal/crt"
	"a.yandex-team.ru/cloud/mdb/internal/crt/letsencrypt/challenge"
	cryptoutils "a.yandex-team.ru/cloud/mdb/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/library/go/core/log"
)

type LetsEncryptClient struct {
	acmeClient        *acme.Client
	challengeProvider challenge.ChallengeProvider
	storage           URLStorage
	l                 log.Logger
	config            Config

	regMutex   sync.Mutex
	registered bool
}

var _ crt.Client = &LetsEncryptClient{}

type Config struct {
	ACMEClient     httputil.ClientConfig `json:"acme_client" yaml:"acme_client"`
	PrivateKeyPath string                `json:"private_key_path" yaml:"private_key_path"`
	Contacts       []string              `json:"contacts" yaml:"contacts"`
}

func DefaultConfig() Config {
	return Config{
		ACMEClient: httputil.ClientConfig{Name: acme.LetsEncryptURL},
	}
}

func New(config Config, challengeProvider challenge.ChallengeProvider, urlStorage URLStorage, l log.Logger) (*LetsEncryptClient, error) {
	httpClient, err := httputil.NewClient(config.ACMEClient, l)
	if err != nil {
		return nil, err
	}

	keyBytes, err := ioutil.ReadFile(config.PrivateKeyPath)
	if err != nil {
		return nil, err
	}

	privateKey, err := cryptoutils.NewPrivateKey(keyBytes)
	if err != nil {
		return nil, err
	}

	acmeClient := &acme.Client{
		HTTPClient:   httpClient.Client,
		Key:          (*rsa.PrivateKey)(privateKey),
		DirectoryURL: config.ACMEClient.Name,
	}

	return &LetsEncryptClient{
		acmeClient:        acmeClient,
		challengeProvider: challengeProvider,
		storage:           urlStorage,
		l:                 l,
		config:            config,
		registered:        false,
	}, nil
}

func (c *LetsEncryptClient) register(ctx context.Context) error {
	c.regMutex.Lock()
	defer c.regMutex.Unlock()

	if c.registered {
		return nil
	}

	var err error
	acc := &acme.Account{Contact: c.config.Contacts}
	_, err = c.acmeClient.Register(ctx, acc, acme.AcceptTOS)
	if err != nil {
		if err == acme.ErrAccountAlreadyExists {
			acc, err = c.acmeClient.GetReg(ctx, "") // Works only with RFC compliant CA
			if err != nil {
				return err
			}
		} else {
			c.l.Errorf("Failed to register in acme server: %e", err)
			return err
		}
	}

	c.l.Infof("Successfully registered with account %+v", *acc)
	c.registered = true
	return nil
}

func (c *LetsEncryptClient) ExistingCert(ctx context.Context, target string, caName string, altNames []string) (*crt.Cert, error) {
	// Certificate renewal goes through new certificate request.
	// Can't retrieve existing due to private key stored only in secretsdb.
	return nil, crt.ErrNoCerts
}
