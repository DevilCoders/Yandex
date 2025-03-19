package clickhouse_test

import (
	"context"
	"fmt"
	"os"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/chutil"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore/clickhouse"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const (
	portEnvVar = "RECIPE_CLICKHOUSE_NATIVE_PORT"

	hostHealthTimeout = 15 * time.Second
)

func initClickhouse(t *testing.T) healthstore.Backend {
	port := os.Getenv(portEnvVar)
	require.NotEmpty(t, port)

	logger, err := zap.New(zap.ConsoleConfig(log.TraceLevel))
	require.NoError(t, err)
	require.NotNil(t, logger)

	b, err := clickhouse.New(logger, chutil.Config{Addrs: []string{fmt.Sprintf("localhost:%s", port)}, Debug: true})
	require.NoError(t, err)
	return b
}

func TestIsReady(t *testing.T) {
	b := initClickhouse(t)
	require.NoError(t, b.IsReady(context.Background()))
}
