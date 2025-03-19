package postgres

import (
	"context"
	"embed"
	"log"

	"github.com/jackc/pgx/v4/pgxpool"

	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	sr_config "a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/config"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/transfer_manager/go/pkg/config"
	pgcommon "a.yandex-team.ru/transfer_manager/go/pkg/dataagent/pg"
	"a.yandex-team.ru/transfer_manager/go/pkg/dbaas"
)

//go:embed migrations
var migrationFs embed.FS

const (
	resourcePath = "migrations"
)

// DB represents postgres database instance
type DB struct {
	*pgxpool.Pool
}

func resolvePgHA(connConfig sr_config.PgConfig) (*dbaas.PgHA, error) {
	switch dbDiscovery := connConfig.Discover.(type) {
	case *config.OnPremise:
		pg, err := dbaas.NewPgHAFromHosts(
			connConfig.Database,
			string(connConfig.User),
			string(connConfig.Password),
			[]string{dbDiscovery.Host},
			dbDiscovery.Port,
			dbDiscovery.UseTLS,
		)
		if err != nil {
			return nil, xerrors.Errorf("unable to create pg HA wrapper from hosts: %w", err)
		}
		return pg, nil
	case *config.MDB:
		pg, err := dbaas.NewPgHAFromMDB(
			connConfig.Database,
			string(connConfig.User),
			string(connConfig.Password),
			dbDiscovery.ClusterID,
			"",
		)
		if err != nil {
			return nil, xerrors.Errorf("unable to create pg HA wrapper from mdb: %w", err)
		}
		return pg, nil
	default:
		return nil, xerrors.Errorf("unknow db config: %T", connConfig)
	}
}

// NewStore create a postgres store
func NewStore(conn sr_config.PgConfig) *Store {
	pgHA, err := resolvePgHA(conn)
	if err != nil {
		log.Fatal(err)
	}
	connConfig, err := pgHA.ConnString(dbaas.MASTER)
	if err != nil {
		log.Fatal(err)
	}
	cc, _ := pgxpool.ParseConfig(connConfig)
	cc.ConnConfig.PreferSimpleProtocol = true
	pgcommon.WithLogger(cc.ConnConfig, logger.Log)

	pgxPool, err := pgxpool.ConnectConfig(context.Background(), cc)
	if err != nil {
		log.Fatal(err)
	}

	return &Store{
		db: &DB{Pool: pgxPool},
	}
}

// NewHTTPFSMigrator reads the migrations from httpfs and returns the migrate.Migrate
//func NewHTTPFSMigrator(DBConnURL string) (*migrate.Migrate, error) {
//	src, err := httpfs.New(http.FS(migrationFs), resourcePath)
//	if err != nil {
//		return &migrate.Migrate{}, fmt.Errorf("db migrator: %v", err)
//	}
//	return migrate.NewWithSourceInstance("httpfs", src, DBConnURL)
//}

// Migrate to run up migrations
func Migrate(connConfig sr_config.PgConfig) error {
	//m, err := NewHTTPFSMigrator(connURL)
	//if err != nil {
	//	return errors.Wrap(err, "db migrator")
	//}
	//defer m.Close()
	//
	//if err := m.Up(); err != nil && err != migrate.ErrNoChange {
	//	return errors.Wrap(err, "db migrator")
	//}
	return nil
}
