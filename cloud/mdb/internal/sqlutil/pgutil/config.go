package pgutil

import (
	"fmt"
	"net"
	"strings"
	"time"

	"github.com/jackc/pgx/v4"
	"github.com/jackc/pgx/v4/stdlib"

	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/x/net/dial"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	defaultSSLMode = RequireSSLMode
)

// RegisterConfigForConnString with default values we use
func RegisterConfigForConnString(connString string, tcpcfg dial.TCPConfig) (string, error) {
	cfg, err := pgx.ParseConfig(connString)
	if err != nil {
		return "", xerrors.Errorf("failed to parse pgx config: %w", err)
	}

	// Configure network dialers
	cfg.DialFunc = tcpcfg.Connect.Dialer().DialContext
	cfg.LookupFunc = tcpcfg.DNSLookup.Resolver().LookupHost

	// TODO: investigate "statement_cache_mode: describe"
	cfg.PreferSimpleProtocol = true
	return stdlib.RegisterConnConfig(cfg), nil
}

// Config represents PostgreSQL configuration (one or more nodes)
type Config struct {
	Addrs       []string       `json:"addrs" yaml:"addrs"`
	DB          string         `json:"db" yaml:"db"`
	User        string         `json:"user" yaml:"user"`
	Password    secret.String  `json:"password" yaml:"password"`
	SSLMode     sslMode        `json:"sslmode" yaml:"sslmode"`
	SSLRootCert string         `json:"sslrootcert" yaml:"sslrootcert"`
	TCP         dial.TCPConfig `json:"tcp" yaml:"tcp"`
	MaxOpenConn int            `json:"max_open_conn" yaml:"max_open_conn"`
	MaxIdleConn int            `json:"max_idle_conn" yaml:"max_idle_conn"`
	MaxIdleTime time.Duration  `json:"max_idle_time" yaml:"max_idle_time"`
	MaxLifetime time.Duration  `json:"max_lifetime" yaml:"max_lifetime"`
}

func DefaultConfig() Config {
	return Config{
		TCP:         dial.DefaultTCPConfig(),
		MaxOpenConn: 32,
		MaxIdleConn: 32,
		MaxIdleTime: 15 * time.Minute,
	}
}

// String implements Stringer
func (c Config) String() string {
	return fmt.Sprintf("Addrs: '%s' DB: '%s' User: '%s' SSLRootCert: '%s'", c.Addrs, c.DB, c.User, c.SSLRootCert)
}

func (c Config) Validate() error {
	if len(c.Addrs) == 0 {
		return xerrors.New("no addresses provided")
	}

	return nil
}

// ConnString constructs PostgreSQL connection string
func ConnString(addr, dbname, user, password, sslMode, sslRootCert string) string {
	var connParams []string

	host, port, err := net.SplitHostPort(addr)
	if err == nil {
		connParams = append(connParams, "host="+host)
		connParams = append(connParams, "port="+port)
	} else {
		connParams = append(connParams, "host="+addr)
	}

	if dbname != "" {
		connParams = append(connParams, "dbname="+dbname)
	}

	if user != "" {
		connParams = append(connParams, "user="+user)
	}

	if password != "" {
		connParams = append(connParams, "password="+password)
	}

	if sslRootCert != "" {
		connParams = append(connParams, "sslrootcert="+sslRootCert)
		//if CA cert is present and mode not specified then verify-full
		if sslMode == "" {
			sslMode = VerifyFullSSLMode
		}
	}

	if sslMode != "" {
		connParams = append(connParams, "sslmode="+sslMode)
	} else {
		connParams = append(connParams, "sslmode="+defaultSSLMode)
	}

	return strings.Join(connParams, " ")
}
