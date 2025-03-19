package recipeconfig

import (
	"fmt"
	"os"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func getEnvVar(varName, name string) (string, error) {
	key := strings.ToUpper(name) + "_" + varName
	val, ok := os.LookupEnv(key)
	if !ok {
		return "", xerrors.Errorf("%s is missing", key)
	}
	return val, nil
}

// New construct config for cluster created from recipe
func New(clusterName, dbName, user string) (pgutil.Config, error) {
	host, err := getEnvVar("POSTGRESQL_RECIPE_HOST", clusterName)
	if err != nil {
		return pgutil.Config{}, err
	}
	port, err := getEnvVar("POSTGRESQL_RECIPE_PORT", clusterName)
	if err != nil {
		return pgutil.Config{}, err
	}
	addr := fmt.Sprintf("%s:%s", host, port)
	return pgutil.Config{
		Addrs:   []string{addr},
		SSLMode: pgutil.DisableSSLMode,
		DB:      dbName,
		User:    user,
	}, nil
}
