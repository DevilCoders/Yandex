package integrationtests

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/dbteststeps"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func initPG(t *testing.T) (context.Context, cmsdb.Client, log.Logger) {
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
	cfg.Addrs = []string{dbteststeps.DBHostPort("CMSDB")}
	cfg.DB = "cmsdb"
	cfg.User = "cms"
	return cfg
}

func tearDown(t *testing.T, db cmsdb.Client) {
	tables := []string{
		"requests",
		"decisions",
		"instance_operations",
	}
	for _, table := range tables {
		execQuery(t, db, fmt.Sprintf("DELETE FROM cms.%v;", table))
	}
	require.NoError(t, db.Close())
}

func waitForBackend(ctx context.Context, t *testing.T, db cmsdb.Client, l log.Logger) {
	require.NoError(t, ready.WaitWithTimeout(ctx, 20*time.Second, db, &ready.DefaultErrorTester{Name: "vpc database", L: l}, time.Second))
}

func execQuery(t *testing.T, db cmsdb.Client, query string) {
	c := db.(*pg.Backend)
	_, err := c.GetDB().Primary().DB().Exec(query)
	require.NoError(t, err)
}
