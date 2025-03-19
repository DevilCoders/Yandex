package helpers

import (
	"fmt"
	"os"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	envNameHost = "DEPLOYDB_POSTGRESQL_RECIPE_HOST"
	envNamePort = "DEPLOYDB_POSTGRESQL_RECIPE_PORT"
)

func Host() (string, error) {
	host, ok := os.LookupEnv(envNameHost)
	if !ok {
		return "", xerrors.Errorf("deploydb recipe host %q missing in env", envNameHost)
	}

	return host, nil
}

func MustHost() string {
	host, err := Host()
	if err != nil {
		panic(err)
	}

	return host
}

func Port() (string, error) {
	port, ok := os.LookupEnv(envNamePort)
	if !ok {
		return "", xerrors.Errorf("deploydb recipe port %q missing in env", envNamePort)
	}

	return port, nil
}

func MustPort() string {
	port, err := Port()
	if err != nil {
		panic(err)
	}

	return port
}

func PGConfig() pgutil.Config {
	return pgutil.Config{
		Addrs:   []string{fmt.Sprintf("%s:%s", MustHost(), MustPort())},
		DB:      "deploydb",
		SSLMode: pgutil.AllowSSLMode,
		User:    "deploy_api",
	}
}
