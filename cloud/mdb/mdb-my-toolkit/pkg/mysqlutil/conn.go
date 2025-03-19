package mysqlutil

import (
	"crypto/tls"
	"crypto/x509"
	"errors"
	"fmt"
	"io/ioutil"
	"net"
	"os/user"
	"path"
	"strconv"
	"strings"

	"github.com/go-sql-driver/mysql"
	"github.com/jmoiron/sqlx"
	"gopkg.in/ini.v1"
)

const (
	defaultConfigFile = ".my.cnf"
	protoUnix         = "unix"
	protoTCP          = "tcp"
)

const (
	sslModeDisabled       = "DISABLED"
	sslModePrefered       = "PREFERRED"
	sslModeRequired       = "REQUIRED"
	sslModeVerifyCA       = "VERIFY_CA"
	sslModeVerifyIdentity = "VERIFY_IDENTITY"
)

var sslModeMap = map[string]string{
	sslModeDisabled:       "false",
	sslModePrefered:       "skip-verify",
	sslModeRequired:       "skip-verify",
	sslModeVerifyCA:       "true",
	sslModeVerifyIdentity: "true",
}

// MyCnf represents MySQL client configuration, stored typically in .my.cnf files
type MyCnf struct {
	User           string
	Password       string
	Host           string
	Port           int
	Database       string
	Socket         string
	SslMode        string
	SslCA          string
	ConnectTimeout int
}

// ReadDefaultsFile reads MySQL client configuration from cnfPath
// If cnfPath is empty, ~/.my.cnf is used
func ReadDefaultsFile(cnfPath string) (*MyCnf, error) {
	if cnfPath == "" {
		usr, err := user.Current()
		if err != nil {
			return nil, err
		}
		cnfPath = path.Join(usr.HomeDir, defaultConfigFile)
	}
	cfg, err := ini.Load(cnfPath)
	if err != nil {
		return nil, err
	}
	settings := cfg.Section("client")
	if settings == nil {
		return nil, errors.New("invalid syntax: client section is missed")
	}
	cnf := new(MyCnf)
	err = cnf.setFromIni(settings)
	if err != nil {
		return nil, err
	}
	err = cnf.setDefaults()
	if err != nil {
		return nil, err
	}
	if cnf.SslCA != "" && cnf.SslMode != sslModeDisabled {
		err = cnf.loadCert()
		if err != nil {
			return nil, err
		}
	}
	return cnf, nil
}

// nolint: gocyclo
func (cnf *MyCnf) setFromIni(settings *ini.Section) error {
	var err error
	for _, k := range settings.Keys() {
		switch k.Name() {
		case "host":
			cnf.Host = k.String()
		case "port":
			cnf.Port, err = k.Int()
			if err != nil {
				return errors.New("invalid syntax: port not a number")
			}
		case "socket":
			cnf.Socket = k.String()
		case "database":
			cnf.Database = k.String()
		case "user":
			cnf.User = k.String()
		case "password":
			cnf.Password = k.String()
		case "ssl-mode":
			cnf.SslMode = k.String()
			if _, ok := sslModeMap[cnf.SslMode]; cnf.SslMode != "" && !ok {
				return errors.New("invalid syntax: invalid ssl-mode setting")
			}
		case "ssl-ca":
			cnf.SslCA = k.String()
		case "connect-timeout":
			cnf.ConnectTimeout, err = k.Int()
			if err != nil {
				return errors.New("invalid syntax: connect-timeout not a number")
			}
		}
	}
	if cnf.Host == "" && cnf.Socket == "" {
		return errors.New("invalid syntax: either socket or host should be specified")
	}
	return nil
}

func (cnf *MyCnf) setDefaults() error {
	if cnf.Host != "" {
		if cnf.Port == 0 {
			cnf.Port = 3306
		}
	}
	if cnf.User == "" {
		usr, err := user.Current()
		if err != nil {
			return err
		}
		cnf.User = usr.Username
	}
	if cnf.SslMode == "" {
		cnf.SslMode = sslModePrefered
	}
	return nil
}

func (cnf *MyCnf) loadCert() error {
	var pem []byte
	rootCertPool := x509.NewCertPool()
	pem, err := ioutil.ReadFile(cnf.SslCA)
	if err != nil {
		return err
	}
	if ok := rootCertPool.AppendCertsFromPEM(pem); !ok {
		return fmt.Errorf("failed to parse PEM certificate")
	}
	tlsConfig := &tls.Config{
		RootCAs:            rootCertPool,
		InsecureSkipVerify: cnf.SslMode == sslModePrefered || cnf.SslMode == sslModeRequired,
	}
	err = mysql.RegisterTLSConfig("custom", tlsConfig)
	if err != nil {
		return err
	}
	return nil
}

// Dsn returns connection string configured in my.cnf file
func (cnf *MyCnf) Dsn() string {
	return cnf.DsnWithHost("")
}

// DsnWithHost returns the same connection string as Dsn, but with host substituted
// Useful for connecting to different nodes in the same MySQL cluster
func (cnf *MyCnf) DsnWithHost(host string) string {
	cred := cnf.User
	if cnf.Password != "" {
		cred += ":" + cnf.Password
	}
	var proto, addr string
	if cnf.Host != "" || host != "" {
		if host == "" {
			host = cnf.Host
		}
		proto = protoTCP
		addr = net.JoinHostPort(host, strconv.Itoa(cnf.Port))
	} else {
		proto = protoUnix
		addr = cnf.Socket
	}
	opts := []string{}
	if cnf.ConnectTimeout > 0 {
		opts = append(opts, fmt.Sprintf("timeout=%d", cnf.ConnectTimeout))
	}
	if proto != protoUnix {
		if cnf.SslCA != "" && cnf.SslMode != sslModeDisabled {
			opts = append(opts, "tls=custom")
		} else if cnf.SslMode != "" {
			opts = append(opts, "tls="+sslModeMap[cnf.SslMode])
		}
	}
	dsn := fmt.Sprintf("%s(%s)/", proto, addr)
	if cred != "" {
		dsn = cred + "@" + dsn
	}
	if cnf.Database != "" {
		dsn = dsn + cnf.Database
	} else {
		dsn = dsn + "mysql"
	}
	if len(opts) > 0 {
		dsn = dsn + "?" + strings.Join(opts, "&")
	}
	return dsn
}

// ConnectWithDefaultsFile reads client config and returns sqlx-wrapped connection handler
func ConnectWithDefaultsFile(cnfPath string) (*sqlx.DB, error) {
	return ConnectWithDefaultsFileAndHost(cnfPath, "")
}

// ConnectWithDefaultsFileAndHost is the same as ConnectWithDefaultsFile, but with host substituted
func ConnectWithDefaultsFileAndHost(cnfPath string, host string) (*sqlx.DB, error) {
	cnf, err := ReadDefaultsFile(cnfPath)
	if err != nil {
		return nil, err
	}
	conn, err := sqlx.Open("mysql", cnf.DsnWithHost(host))
	if err != nil {
		return nil, err
	}
	conn = conn.Unsafe()
	return conn, nil
}
