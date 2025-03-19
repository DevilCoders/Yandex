package basic

import (
	"os"
	"strconv"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/transfer_manager/go/pkg/dbaas"
)

func NewRecipePG() (*dbaas.PgHA, error) {
	if os.Getenv("PG_LOCAL_CLUSTER_ID") != "" {
		_ = dbaas.InitializeInternalCloud("https://gw.db.yandex-team.ru/", "https://gw.db.yandex-team.ru:443/iam/v1/", os.Getenv("DBAAS_TOKEN"))
		return dbaas.NewPgHAFromMDB(
			os.Getenv("PG_LOCAL_DATABASE"),
			os.Getenv("PG_LOCAL_USER"),
			os.Getenv("PG_LOCAL_PASSWORD"),
			os.Getenv("PG_LOCAL_CLUSTER_ID"),
			os.Getenv("DBAAS_TOKEN"),
		)
	}
	port, err := strconv.Atoi(os.Getenv("PG_LOCAL_PORT"))
	if err != nil {
		return nil, xerrors.Errorf("Cannot parse DB port: %w", err)
	}
	pg, err := dbaas.NewPgHAFromHosts(
		os.Getenv("PG_LOCAL_DATABASE"),
		os.Getenv("PG_LOCAL_USER"),
		os.Getenv("PG_LOCAL_PASSWORD"),
		[]string{"localhost"},
		port,
		false, // TLS disabled
	)
	if err != nil {
		return nil, xerrors.Errorf("Cannot create connection config: %w", err)
	}
	return pg, nil
}
