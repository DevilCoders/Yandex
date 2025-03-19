package chutil

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"io/ioutil"
	"strings"

	"github.com/ClickHouse/clickhouse-go"
	"github.com/gofrs/uuid"

	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Config represents ClickHouse configuration (one or more nodes)
type Config struct {
	Addrs    []string      `json:"addrs" yaml:"addrs"`
	DB       string        `json:"db" yaml:"db"`
	User     string        `json:"user" yaml:"user"`
	Password secret.String `json:"password" yaml:"password"`
	Secure   bool          `json:"secure" yaml:"secure"`
	Compress bool          `json:"compress" yaml:"compress"`
	Debug    bool          `json:"debug" yaml:"debug"`
	CAFile   string        `json:"ca_file" yaml:"ca_file"`

	tlsConfigName string
}

// String implements Stringer
func (c *Config) String() string {
	return fmt.Sprintf("Addrs: %q DB: %q User: %q Secure: %t Compress %t Debug %t", c.Addrs, c.DB, c.User, c.Secure, c.Compress, c.Debug)
}

func (c *Config) Validate() error {
	if len(c.Addrs) == 0 {
		return xerrors.New("no addresses provided")
	}

	return nil
}

func (c *Config) RegisterTLSConfig() error {
	u, err := uuid.NewV4()
	if err != nil {
		return xerrors.Errorf("failed to generate unique tls config name: %w", err)
	}

	c.tlsConfigName = u.String()
	tlsConfig := &tls.Config{}

	if c.CAFile != "" {
		cafile, err := ioutil.ReadFile(c.CAFile)
		if err != nil {
			return xerrors.Errorf("failed to read certificate from %q: %w", c.CAFile, err)
		}

		cpool := x509.NewCertPool()
		if !cpool.AppendCertsFromPEM(cafile) {
			return xerrors.Errorf("failed to append certificate from %q to cert pool", c.CAFile)
		}

		tlsConfig = &tls.Config{
			RootCAs:   cpool,
			ClientCAs: cpool,
		}
	}

	if err = clickhouse.RegisterTLSConfig(c.tlsConfigName, tlsConfig); err != nil {
		return xerrors.Errorf("failed to register tls config: %w", err)
	}

	return nil
}

func (c *Config) URI() string {
	uri := fmt.Sprintf("tcp://%s?database=%s&username=%s&password=%s&secure=%t&compress=%t&debug=%t",
		c.Addrs[0],
		c.DB,
		c.User,
		c.Password.Unmask(),
		c.Secure,
		c.Compress,
		c.Debug,
	)

	if c.tlsConfigName != "" {
		uri = fmt.Sprintf("%s&tls_config=%s", uri, c.tlsConfigName)
	}

	if len(c.Addrs) > 1 {
		uri = fmt.Sprintf("%s&alt_hosts=%s", uri, strings.Join(c.Addrs[1:], ","))
	}

	return uri
}
