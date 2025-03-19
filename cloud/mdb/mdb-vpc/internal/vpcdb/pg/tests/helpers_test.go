package tests_test

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/dbteststeps"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/pg"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func initPG(t *testing.T) (context.Context, vpcdb.VPCDB, log.Logger) {
	ctx := context.Background()
	logger, _ := zap.New(zap.ConsoleConfig(log.DebugLevel))

	cfg := pgConfig()
	db, err := pg.New(cfg, logger)
	require.NoError(t, err)
	require.NotNil(t, db)
	return ctx, db, logger
}

func pgConfig() pgutil.Config {
	cfg := pgutil.DefaultConfig()
	cfg.Addrs = []string{dbteststeps.DBHostPort("VPCDB")}
	cfg.DB = "vpcdb"
	cfg.User = "vpc_worker"
	return cfg
}

func tearDown(t *testing.T, db vpcdb.VPCDB) {
	tables := []string{
		"network_connections",
		"networks",
		"operations",
	}
	for _, table := range tables {
		execQuery(t, db, fmt.Sprintf("DELETE FROM vpc.%v;", table))
	}

}

func waitForBackend(ctx context.Context, t *testing.T, db vpcdb.VPCDB, l log.Logger) {
	require.NoError(t, ready.WaitWithTimeout(ctx, 20*time.Second, db, &ready.DefaultErrorTester{Name: "vpc database", L: l}, time.Second))
}

func execQuery(t *testing.T, db vpcdb.VPCDB, query string) {
	c := db.(*pg.DB)
	_, err := c.GetDB().Primary().DB().Exec(query)
	require.NoError(t, err)
}
