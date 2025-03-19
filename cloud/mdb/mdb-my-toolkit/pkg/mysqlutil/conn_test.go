package mysqlutil

import (
	"io/ioutil"
	"log"
	"os"
	"testing"

	"github.com/stretchr/testify/require"
)

func tempConfig(data string) string {
	file, err := ioutil.TempFile("/tmp", "my.cnf.test")
	if err != nil {
		log.Fatal(err)
	}
	err = ioutil.WriteFile(file.Name(), []byte(data), 0600)
	if err != nil {
		_ = os.Remove(file.Name())
		log.Fatal(err)
	}
	return file.Name()
}

func TestReadDefaultsFile(t *testing.T) {
	path := tempConfig(`
    ssl-ca = /some/trash
	[client]
	# does not matter
	user = admin

	password = admin_pwd
	host = host.net
	port = 3307

	[other]
	port = 3306
	`)
	defer func() { _ = os.Remove(path) }()

	cnf, err := ReadDefaultsFile(path)
	if err != nil {
		t.Errorf("failed to read config: %v", err)
	}
	require.Equal(t, "admin", cnf.User, "user field matches")
	require.Equal(t, "admin_pwd", cnf.Password, "password field matches")
	require.Equal(t, "host.net", cnf.Host, "host field matches")
	require.Equal(t, 3307, cnf.Port, "port field matches")
	require.Equal(t, "", cnf.Socket, "socket field should be empty")
	require.Equal(t, "PREFERRED", cnf.SslMode, "ssl-mode should have default")
	require.Equal(t, "", cnf.SslCA, "ssl-ca should be empty")
	require.Equal(t, "", cnf.Database, "database should be empty")
}

func TestDsn(t *testing.T) {
	cnf := &MyCnf{
		User:           "admin",
		Password:       "admin_pwd",
		Host:           "localhost",
		Port:           3305,
		Database:       "sys",
		SslMode:        "VERIFY_IDENTITY",
		SslCA:          "/path/to/some.crt",
		ConnectTimeout: 5,
	}
	require.Equal(
		t,
		"admin:admin_pwd@tcp(localhost:3305)/sys?timeout=5&tls=custom",
		cnf.Dsn(),
		"dsn with tcp should match",
	)
	cnf = &MyCnf{
		User:    "admin",
		Socket:  "/var/run/mysqld/mysqld.sock",
		SslMode: "REQUIRED",
	}
	require.Equal(
		t,
		"admin@unix(/var/run/mysqld/mysqld.sock)/mysql",
		cnf.Dsn(),
		"dsn with unix socket should match",
	)
}
