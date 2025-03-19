package integration

import (
	"context"
	"database/sql"
	_ "embed"
	"os"
	"runtime"
	"strings"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/chutil"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/logsdb"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/logsdb/clickhouse"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

//go:embed schema/datacloud.sql
var clickhouseSchema string

type queryTestArgs struct {
	Name             string
	InsertDataQuery  string
	InsertDataParams [][]interface{}
	Criteria         logsdb.Criteria
	Result           []models.Log
}

func TestClickhouseQueries(t *testing.T) {
	ctx, db, back := setupTestEnv(t)
	queryIDFilter, _ := sqlfilter.Parse("query_id = '123'")
	inputs := []queryTestArgs{
		{
			Name:            "Read all ClickHouse logs",
			InsertDataQuery: "INSERT INTO mdb.clickhouse_part(timestamp, ms, cluster, severity, message, hostname) VALUES (?, ?, ?, ?, ?, ?)",
			InsertDataParams: [][]interface{}{
				{time.Date(2021, time.January, 1, 0, 0, 0, 0, time.Local), 100, "cid1", "Debug", "msg1", "fqdn1"},
				{time.Date(2021, time.January, 1, 0, 1, 0, 0, time.Local), 100, "cid1", "Warning", "msg2", "fqdn2"},
			},
			Criteria: logsdb.Criteria{
				Sources:     []models.LogSource{{Type: models.LogSourceTypeClickhouse, ID: "cid1"}},
				FromSeconds: time.Date(2021, time.January, 1, 0, 0, 0, 0, time.Local).Unix(),
				FromMS:      0,
				ToSeconds:   time.Date(2021, time.January, 1, 0, 3, 0, 0, time.Local).Unix(),
				ToMS:        0,
				Order:       models.SortOrderAscending,
				Offset:      0,
				Limit:       100,
			},
			Result: []models.Log{
				{Source: models.LogSource{Type: models.LogSourceTypeClickhouse, ID: "cid1"}, Instance: optional.NewString("fqdn1"), Timestamp: time.Date(2021, time.January, 1, 0, 0, 0, 100*1e6, time.Local), Severity: models.LogLevelDebug, Message: "msg1", Offset: 1},
				{Source: models.LogSource{Type: models.LogSourceTypeClickhouse, ID: "cid1"}, Instance: optional.NewString("fqdn2"), Timestamp: time.Date(2021, time.January, 1, 0, 1, 0, 100*1e6, time.Local), Severity: models.LogLevelWarning, Message: "msg2", Offset: 2},
			},
		},
		{
			Name:            "Read all ClickHouse logs with filters",
			InsertDataQuery: "INSERT INTO mdb.clickhouse_part(timestamp, ms, cluster, severity, message, hostname, query_id) VALUES (?, ?, ?, ?, ?, ?, ?)",
			InsertDataParams: [][]interface{}{
				{time.Date(2021, time.January, 1, 0, 0, 0, 0, time.Local), 100, "cid1", "Debug", "msg1", "fqdn1", "122"},
				{time.Date(2021, time.January, 1, 0, 0, 0, 0, time.Local), 100, "cid1", "Debug", "msg3", "fqdn1", "123"},
				{time.Date(2021, time.January, 1, 0, 1, 0, 0, time.Local), 100, "cid1", "Warning", "msg2", "fqdn2", "120"},
			},
			Criteria: logsdb.Criteria{
				Sources:     []models.LogSource{{Type: models.LogSourceTypeClickhouse, ID: "cid1"}},
				FromSeconds: time.Date(2021, time.January, 1, 0, 0, 0, 0, time.Local).Unix(),
				FromMS:      0,
				ToSeconds:   time.Date(2021, time.January, 1, 0, 3, 0, 0, time.Local).Unix(),
				ToMS:        0,
				Order:       models.SortOrderAscending,
				Offset:      0,
				Limit:       100,
				Levels:      []models.LogLevel{models.LogLevelDebug},
				Filters:     queryIDFilter,
			},
			Result: []models.Log{
				{Source: models.LogSource{Type: models.LogSourceTypeClickhouse, ID: "cid1"}, Instance: optional.NewString("fqdn1"), Timestamp: time.Date(2021, time.January, 1, 0, 0, 0, 100*1e6, time.Local), Severity: models.LogLevelDebug, Message: "msg3", Offset: 1},
			},
		},
		{
			Name:            "Read all DataTransfer logs",
			InsertDataQuery: "INSERT INTO mdb.data_transfer_dp(_timestamp, id, task_id, level, msg, host) VALUES (?, ?, ?, ?, ?, ?)",
			InsertDataParams: [][]interface{}{
				{time.Date(2021, time.January, 1, 0, 0, 0, 100*1e6, time.Local), "dtt123", "dtj123", "INFO", "msg1", nil},
				{time.Date(2021, time.January, 1, 0, 1, 0, 100*1e6, time.Local), "dtt123", "dtj123", "WARN", "msg2", nil},
			},
			Criteria: logsdb.Criteria{
				Sources:     []models.LogSource{{Type: models.LogSourceTypeTransfer, ID: "dtt123"}},
				FromSeconds: time.Date(2021, time.January, 1, 0, 0, 0, 0, time.Local).Unix(),
				FromMS:      0,
				ToSeconds:   time.Date(2021, time.January, 1, 0, 3, 0, 0, time.Local).Unix(),
				ToMS:        0,
				Order:       models.SortOrderAscending,
				Offset:      0,
				Limit:       100,
			},
			Result: []models.Log{
				{Source: models.LogSource{Type: models.LogSourceTypeTransfer, ID: "dtt123"}, Instance: optional.String{}, Timestamp: time.Date(2021, time.January, 1, 0, 0, 0, 100*1e6, time.Local), Severity: models.LogLevelInfo, Message: "msg1", Offset: 1},
				{Source: models.LogSource{Type: models.LogSourceTypeTransfer, ID: "dtt123"}, Instance: optional.String{}, Timestamp: time.Date(2021, time.January, 1, 0, 1, 0, 100*1e6, time.Local), Severity: models.LogLevelWarning, Message: "msg2", Offset: 2},
			},
		},
		{
			Name:            "Read all Kafka logs",
			InsertDataQuery: "INSERT INTO mdb.kafka_part(timestamp, ms, cluster, severity, message, hostname) VALUES (?, ?, ?, ?, ?, ?)",
			InsertDataParams: [][]interface{}{
				{time.Date(2021, time.January, 1, 0, 0, 0, 0, time.Local), 100, "cid1", "Debug", "msg1", "fqdn1"},
				{time.Date(2021, time.January, 1, 0, 1, 0, 0, time.Local), 100, "cid1", "Warning", "msg2", "fqdn2"},
			},
			Criteria: logsdb.Criteria{
				Sources:     []models.LogSource{{Type: models.LogSourceTypeKafka, ID: "cid1"}},
				FromSeconds: time.Date(2021, time.January, 1, 0, 0, 0, 0, time.Local).Unix(),
				FromMS:      0,
				ToSeconds:   time.Date(2021, time.January, 1, 0, 3, 0, 0, time.Local).Unix(),
				ToMS:        0,
				Order:       models.SortOrderAscending,
				Offset:      0,
				Limit:       100,
			},
			Result: []models.Log{
				{Source: models.LogSource{Type: models.LogSourceTypeKafka, ID: "cid1"}, Instance: optional.NewString("fqdn1"), Timestamp: time.Date(2021, time.January, 1, 0, 0, 0, 100*1e6, time.Local), Severity: models.LogLevelDebug, Message: "msg1", Offset: 1},
				{Source: models.LogSource{Type: models.LogSourceTypeKafka, ID: "cid1"}, Instance: optional.NewString("fqdn2"), Timestamp: time.Date(2021, time.January, 1, 0, 1, 0, 100*1e6, time.Local), Severity: models.LogLevelWarning, Message: "msg2", Offset: 2},
			},
		},
	}

	// SessionStats runs
	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			require.NoError(t, cleanupTestData(db))
			tx, err := db.Begin()
			require.NoError(t, err)
			stmt, err := tx.Prepare(input.InsertDataQuery)
			require.NoError(t, err)

			for _, params := range input.InsertDataParams {
				_, err = stmt.Exec(params...)
				require.NoError(t, err)
			}

			require.NoError(t, tx.Commit())
			msg, _, err := back.Logs(ctx, input.Criteria)
			require.NoError(t, err)
			require.Equal(t, input.Result, msg)
		})
	}
}

func setupTestEnv(t *testing.T) (context.Context, *sql.DB, *clickhouse.Backend) {
	chPort, ok := os.LookupEnv("RECIPE_CLICKHOUSE_NATIVE_PORT")
	if !ok {
		if runtime.GOOS == "darwin" {
			t.SkipNow()
		}
		t.Fatal("RECIPE_CLICKHOUSE_NATIVE_PORT unset")
	}
	ctx := context.Background()
	cfg := clickhouse.DefaultConfig()
	cfg.DB.Secure = false
	cfg.DB.User = "default"
	cfg.DB.Addrs = []string{"localhost:" + chPort}
	cfg.DB.CAFile = ""

	db, err := checkTestDBConnection(cfg.DB)
	require.NoError(t, err)

	l, _ := zap.New(zap.KVConfig(log.DebugLevel))
	initDB(t, db)
	cfg.DB.DB = "mdb"
	back, err := clickhouse.New(cfg, l)

	require.NoError(t, err, "backed initialization failed")

	err = back.IsReady(ctx)
	require.NoError(t, err, "backend not ready")

	return ctx, db, back
}

func initDB(t *testing.T, db *sql.DB) {
	// Cleanup
	_, err := db.Exec("DROP DATABASE IF EXISTS mdb")
	require.NoError(t, err)

	createQueries := strings.Split(clickhouseSchema, ";")

	for _, query := range createQueries {
		query = strings.Replace(query, " ON CLUSTER '{cluster}'", "", 1)
		_, err = db.Exec(query)
		require.NoError(t, err, "failed to execute: %q", query)
	}
}

func checkTestDBConnection(cfg chutil.Config) (*sql.DB, error) {
	cfg.DB = "default"
	db, err := sql.Open("clickhouse", cfg.URI())
	if err != nil {
		return nil, err
	}
	err = db.Ping()
	if err != nil {
		return nil, err
	}
	return db, nil
}

var (
	cleanupTestDataQueries = []string{
		"TRUNCATE TABLE mdb.clickhouse_part",
		"TRUNCATE TABLE mdb.kafka_part",
		"TRUNCATE TABLE mdb.data_transfer_dp",
	}
)

func cleanupTestData(db *sql.DB) error {
	for _, q := range cleanupTestDataQueries {
		if _, err := db.Exec(q); err != nil {
			return err
		}
	}

	return nil
}
