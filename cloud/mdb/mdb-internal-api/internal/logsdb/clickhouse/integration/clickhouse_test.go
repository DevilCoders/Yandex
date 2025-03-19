package integration

import (
	"context"
	"database/sql"
	"fmt"
	"os"
	"runtime"
	"strings"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/chutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

var logTypes = []logsdb.LogType{
	logsdb.LogTypePostgreSQL,
	logsdb.LogTypePGBouncer,
	logsdb.LogTypeOdyssey,
	logsdb.LogTypeClickHouse,
	logsdb.LogTypeMongoD,
	logsdb.LogTypeMongoS,
	logsdb.LogTypeMongoCFG,
	logsdb.LogTypeMySQLGeneral,
	logsdb.LogTypeMySQLError,
	logsdb.LogTypeMySQLSlowQuery,
	logsdb.LogTypeMySQLAudit,
	logsdb.LogTypeRedis,
	logsdb.LogTypeElasticsearch,
	logsdb.LogTypeKibana,
}

type inputArgs struct {
	Name    string
	Type    logsdb.LogType
	Columns []string
	Filter  string
	FromTS  time.Time
	ToTS    time.Time
	Limit   int64
	Offset  int64
	Res     []logs.Message
	More    bool
	Err     error
}

type queryTestArgs struct {
	Name             string
	TableName        string
	QueryTemplate    string
	AdditionalParams []interface{}
	Type             logsdb.LogType
	InsertDataQuery  string
	Limit            int64
	Res              []logs.Message
}

func TestClickhouseQueries(t *testing.T) {
	ctx, _, back, err := setupTestEnv(t)
	require.NoError(t, err, "test setup failed")

	inputs := []inputArgs{}

	for _, logType := range logTypes {
		inputs = append(inputs, inputArgs{
			Name:    "Filter_" + string(logType),
			Type:    logType,
			Filter:  "message.hostname='localhost'",
			Limit:   1,
			Columns: nil,
			Res:     nil,
			More:    false,
			Err:     nil,
		})
	}

	// SessionStats runs
	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			f, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			msg, more, err := back.Logs(ctx, "cid1", input.Type,
				input.Columns, input.FromTS, input.ToTS, input.Limit, input.Offset, f)
			require.NoError(t, err)
			require.Equal(t, input.More, more)
			require.Equal(t, input.Res, msg)
		})
	}
}

func TestClickhouseDataFilters(t *testing.T) {
	ctx, cfg, back, err := setupTestEnv(t)
	require.NoError(t, err, "test setup failed")

	insertDataQueryTemplate := "INSERT INTO %s(timestamp, ms, cluster) VALUES (?, ?, ?)"
	insertDataParams := [][]interface{}{
		{time.Date(2021, time.January, 1, 0, 0, 0, 0, time.Local), 100, "cid1"},
		{time.Date(2021, time.January, 1, 0, 1, 0, 0, time.Local), 100, "cid1"},
		{time.Date(2021, time.January, 1, 0, 1, 0, 0, time.Local), 150, "cid1"},
		{time.Date(2021, time.January, 1, 0, 1, 0, 0, time.Local), 200, "cid1"},
		{time.Date(2021, time.January, 1, 0, 2, 0, 0, time.Local), 100, "cid1"},
	}
	fromTS := time.Date(2020, time.December, 31, 23, 59, 59, 533*1e6, time.Local)
	toTS := time.Date(2021, time.January, 1, 0, 1, 0, 150*1e6, time.Local)

	expectedTimestamps := []time.Time{
		time.Date(2021, time.January, 1, 0, 0, 0, 100*1e6, time.Local),
		time.Date(2021, time.January, 1, 0, 1, 0, 100*1e6, time.Local),
	}

	inputs := []queryTestArgs{
		{
			Name:      "Mysql slow log filters ms",
			TableName: "mdb.mysql_slow_query",
			Type:      logsdb.LogTypeMySQLSlowQuery,
			Limit:     4,
		},
		{
			Name:      "Mysql general log filters ms",
			TableName: "mdb.mysql_general",
			Type:      logsdb.LogTypeMySQLGeneral,
			Limit:     4,
		},
		{
			Name:      "Mysql error log filters ms",
			TableName: "mdb.mysql_error",
			Type:      logsdb.LogTypeMySQLError,
			Limit:     4,
		},
		{
			Name:      "Mysql audit log filters ms",
			TableName: "mdb.mysql_audit",
			Type:      logsdb.LogTypeMySQLAudit,
			Limit:     4,
		},
		{
			Name:      "ClickHouse logs filters ms",
			TableName: "mdb.clickhouse",
			Type:      logsdb.LogTypeClickHouse,
			Limit:     4,
		},
		{
			Name:      "Postgres logs filters ms",
			TableName: "mdb.postgres",
			Type:      logsdb.LogTypePostgreSQL,
			Limit:     4,
		},
		{
			Name: "Odyssey logs filters ms",
			// Suddenly, mdb.odyssey is a view
			TableName:     "mdb.odyssey_original",
			QueryTemplate: "INSERT INTO %s(unixtime, ms, cluster) VALUES (?, ?, ?)",
			Type:          logsdb.LogTypeOdyssey,
			Limit:         4,
		},
		{
			Name:      "PGBouncer logs filters ms",
			TableName: "mdb.pgbouncer",
			Type:      logsdb.LogTypePGBouncer,
			Limit:     4,
		},
		{
			Name:      "Redis logs filters ms",
			TableName: "mdb.redis",
			Type:      logsdb.LogTypeRedis,
			Limit:     4,
		},
		{
			Name:      "Kibana logs filters ms",
			TableName: "mdb.kibana",
			Type:      logsdb.LogTypeKibana,
			Limit:     4,
		},
		{
			Name:      "Kafka logs filters ms",
			TableName: "mdb.kafka",
			Type:      logsdb.LogTypeKafka,
			Limit:     4,
		},
		{
			Name:      "Greenplum logs filters ms",
			TableName: "mdb.greenplum",
			Type:      logsdb.LogTypeGreenPlum,
			Limit:     4,
		},
		{
			Name:      "Greenplum Odyssey logs filters ms",
			TableName: "mdb.greenplum_odyssey",
			Type:      logsdb.LogTypeGreenPlumOdyssey,
			Limit:     4,
		},
		{
			Name: "Mongod logs filters ms",
			// mdb.mongod is a view
			TableName:        "mdb.mongodb",
			QueryTemplate:    "INSERT INTO %s(timestamp, ms, cluster, origin) VALUES (?, ?, ?, ?)",
			Type:             logsdb.LogTypeMongoD,
			AdditionalParams: []interface{}{"mongod"},
			Limit:            4,
		},
		{
			Name: "Mongos logs filters ms",
			// mdb.mongos is a view
			TableName:        "mdb.mongodb",
			QueryTemplate:    "INSERT INTO %s(timestamp, ms, cluster, origin) VALUES (?, ?, ?, ?)",
			Type:             logsdb.LogTypeMongoS,
			AdditionalParams: []interface{}{"mongos"},
			Limit:            4,
		},
		{
			Name: "Mongocfg logs filters ms",
			// mdb.mongocfg is a view
			TableName:        "mdb.mongodb",
			QueryTemplate:    "INSERT INTO %s(timestamp, ms, cluster, origin) VALUES (?, ?, ?, ?)",
			Type:             logsdb.LogTypeMongoCFG,
			AdditionalParams: []interface{}{"mongocfg"},
			Limit:            4,
		},
	}

	// SessionStats runs
	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			// Later(?) should be accompanied by 'cleanupTestData'
			queryTemplate := insertDataQueryTemplate
			if input.QueryTemplate != "" {
				queryTemplate = input.QueryTemplate
			}

			query := fmt.Sprintf(queryTemplate, input.TableName)
			err = initTestData(cfg.DB, query, insertDataParams, input.AdditionalParams)
			require.NoError(t, err, "data insertion failed")
			msg, _, err := back.Logs(ctx, "cid1", input.Type,
				nil, fromTS, toTS, input.Limit, 0, nil)
			require.NoError(t, err)

			var timestamps []time.Time
			for _, m := range msg {
				timestamps = append(timestamps, m.Timestamp)
			}

			require.Equal(t, expectedTimestamps, timestamps)
		})
	}
}

func setupTestEnv(t *testing.T) (context.Context, clickhouse.Config, *clickhouse.Backend, error) {
	chPort, ok := os.LookupEnv("RECIPE_CLICKHOUSE_NATIVE_PORT")
	if !ok {
		if runtime.GOOS == "darwin" {
			t.SkipNow()
		}
		t.Fatal("RECIPE_CLICKHOUSE_NATIVE_PORT unset")
	}
	ctx := context.Background()
	cfg := clickhouse.DefaultConfig()
	cfg.DataOpts.TimeColumn = "timestamp"
	cfg.DB.User = "default"
	cfg.DB.Secure = false
	cfg.DB.Addrs = []string{"localhost:" + chPort}
	l, _ := zap.New(zap.KVConfig(log.DebugLevel))
	err := initDB(cfg.DB)
	require.NoError(t, err, "database initialization failed")
	cfg.DB.DB = "mdb"
	back, err := clickhouse.New(cfg, l)

	require.NoError(t, err, "backed initialization failed")

	err = back.IsReady(ctx)
	require.NoError(t, err, "backend not ready")

	return ctx, cfg, back, err
}

func initDB(cfg chutil.Config) error {
	db, err := checkTestDBConnection(cfg)
	if err != nil {
		return err
	}
	createQueries := strings.Split(createQueriesSQL, ";")
	for _, query := range createQueries {
		_, err = db.Exec(query)
		if err != nil {
			return err
		}
	}
	return nil
}

func initTestData(cfg chutil.Config, query string, queryParams [][]interface{}, additionalParams []interface{}) error {
	db, err := checkTestDBConnection(cfg)
	if err != nil {
		return err
	}

	tx, err := db.Begin()
	if err != nil {
		return err
	}

	stmt, err := tx.Prepare(query)
	if err != nil {
		return err
	}

	for _, p := range queryParams {
		if additionalParams != nil {
			p = append(p, additionalParams...)
		}
		_, err = stmt.Exec(p...)
		if err != nil {
			return err
		}
	}

	err = tx.Commit()
	if err != nil {
		return err
	}

	return nil
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

var createQueriesSQL = `
	DROP DATABASE IF EXISTS mdb;
	CREATE DATABASE mdb;
	CREATE TABLE IF NOT EXISTS mdb.postgres
	(
	    timestamp DateTime CODEC(DoubleDelta, LZ4),
	    ms UInt32 DEFAULT 0,
	    cluster LowCardinality(String),
	    hostname LowCardinality(String),
	    origin LowCardinality(String),
	    message String,
	    user_name String,
	    database_name String,
	    process_id UInt32,
	    connection_from String,
	    session_id String,
	    session_line_num UInt32,
	    command_tag String,
	    session_start_time DateTime CODEC(DoubleDelta, LZ4),
	    virtual_transaction_id String,
	    transaction_id UInt64,
	    error_severity LowCardinality(String),
	    sql_state_code LowCardinality(String),
	    detail String,
	    hint String,
	    internal_query String,
	    internal_query_pos UInt32,
	    context String,
	    query String CODEC(ZSTD),
	    query_pos UInt32,
	    location String,
	    application_name String,
	    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
	    log_format LowCardinality(String),
	    log_time DateTime CODEC(DoubleDelta, LZ4),
	    datetime DateTime CODEC(DoubleDelta, LZ4),
	    _timestamp DateTime CODEC(DoubleDelta, LZ4),
	    _partition LowCardinality(String),
	    _offset UInt64,
	    _idx UInt32,
	    _rest String CODEC(ZSTD)
	)
	ENGINE = Memory;
	CREATE TABLE IF NOT EXISTS mdb.pgbouncer
	(
	    timestamp DateTime CODEC(DoubleDelta, LZ4),
	    ms UInt16 DEFAULT 0,
	    cluster LowCardinality(String),
	    hostname LowCardinality(String),
	    origin LowCardinality(String),
	    text String,
	    session_id String,
	    db String,
	    user String,
	    source String,
	    pid UInt32,
	    level String,
	    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
	    datetime DateTime CODEC(DoubleDelta, LZ4),
	    log_format LowCardinality(String),
	    log_time DateTime CODEC(DoubleDelta, LZ4),
	    _timestamp DateTime CODEC(DoubleDelta, LZ4),
	    _partition LowCardinality(String),
	    _offset UInt64,
	    _idx UInt32,
	    _rest String CODEC(ZSTD)
	)
	ENGINE = Memory;
	CREATE TABLE mdb.odyssey_original
	(
	    unixtime DateTime CODEC(DoubleDelta, LZ4),
	    ms UInt32 DEFAULT 0,
	    cluster LowCardinality(String),
	    hostname LowCardinality(String),
	    origin LowCardinality(String),
	    msg String,
	    cid String,
	    sid String,
	    ctx String,
	    db String,
	    user String,
	    pid UInt32,
	    level String,
	    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
	    log_format LowCardinality(String),
	    datetime DateTime CODEC(DoubleDelta, LZ4),
	    _timestamp DateTime CODEC(DoubleDelta, LZ4),
	    _partition LowCardinality(String),
	    _offset UInt64,
	    _idx UInt32,
	    _rest String CODEC(ZSTD(1))
	) ENGINE = Memory;
	CREATE VIEW mdb.odyssey
	(
	    timestamp DateTime,
	    ms UInt32,
	    cluster LowCardinality(String),
	    hostname LowCardinality(String),
	    origin LowCardinality(String),
	    text String,
	    client_id String,
	    server_id String,
	    context String,
	    db String,
	    user String,
	    pid UInt32,
	    level String,
	    insert_time DateTime,
	    log_format LowCardinality(String)
	) AS
	SELECT
	    unixtime AS timestamp,
	    ms AS ms,
	    cluster AS cluster,
	    hostname AS hostname,
	    origin AS origin,
	    msg AS text,
	    cid AS client_id,
	    sid AS server_id,
	    ctx AS context,
	    db AS db,
	    user AS user,
	    pid AS pid,
	    level AS level,
	    insert_time AS insert_time,
	    log_format AS log_format
	FROM mdb.odyssey_original;
	CREATE TABLE IF NOT EXISTS mdb.mongodb
	(
	    timestamp DateTime CODEC(DoubleDelta, LZ4),
	    ms UInt16 DEFAULT 0,
	    cluster LowCardinality(String),
	    hostname LowCardinality(String),
	    origin LowCardinality(String),
	    message String,
	    context String,
	    component LowCardinality(String),
	    severity LowCardinality(String),
	    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
	    log_format LowCardinality(String),
	    datetime DateTime CODEC(DoubleDelta, LZ4),
	    _timestamp DateTime CODEC(DoubleDelta, LZ4),
	    _partition LowCardinality(String),
	    _offset UInt64,
	    _idx UInt32,
	    _rest String CODEC(ZSTD)
	)
	ENGINE = Memory;
	CREATE VIEW IF NOT EXISTS mdb.mongod  AS SELECT * FROM mdb.mongodb WHERE origin = 'mongod';
	CREATE VIEW IF NOT EXISTS mdb.mongos  AS SELECT * FROM mdb.mongodb WHERE origin = 'mongos';
	CREATE VIEW IF NOT EXISTS mdb.mongocfg  AS SELECT * FROM mdb.mongodb WHERE origin = 'mongocfg';
	CREATE TABLE IF NOT EXISTS mdb.clickhouse
	(
	    ms UInt16 DEFAULT 0,
	    cluster LowCardinality(String),
	    hostname LowCardinality(String),
	    origin LowCardinality(String),
	    message String,
	    component String,
	    thread UInt32,
	    severity LowCardinality(String),
	    query_id String,
	    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
	    timestamp DateTime CODEC(DoubleDelta, LZ4),
	    log_format LowCardinality(String),
	    datetime DateTime CODEC(DoubleDelta, LZ4),
	    _timestamp DateTime CODEC(DoubleDelta, LZ4),
	    _partition LowCardinality(String),
	    _offset UInt64,
	    _idx UInt32,
	    _rest String CODEC(ZSTD)
	)
	ENGINE = Memory;
	CREATE TABLE IF NOT EXISTS mdb.redis
	(
	    timestamp DateTime CODEC(DoubleDelta, LZ4),
	    ms UInt16 DEFAULT 0,
	    cluster LowCardinality(String),
	    hostname LowCardinality(String),
	    origin LowCardinality(String),
	    message String,
	    role LowCardinality(String),
	    pid UInt32,
	    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
	    log_format LowCardinality(String),
	    datetime DateTime CODEC(DoubleDelta, LZ4),
	    _timestamp DateTime CODEC(DoubleDelta, LZ4),
	    _partition LowCardinality(String),
	    _offset UInt64,
	    _idx UInt32,
	    _rest String CODEC(ZSTD)
	)
	ENGINE = Memory;
	CREATE TABLE IF NOT EXISTS mdb.mysql_error
	(
	    timestamp DateTime CODEC(DoubleDelta, LZ4),
	    ms UInt16 DEFAULT 0,
	    cluster LowCardinality(String),
	    hostname LowCardinality(String),
	    origin LowCardinality(String),
	    message String,
	    id String,
	    status String,
	    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
	    log_format LowCardinality(String),
	    datetime DateTime CODEC(DoubleDelta, LZ4),
	    raw String CODEC(ZSTD),
	    _timestamp DateTime CODEC(DoubleDelta, LZ4),
	    _partition LowCardinality(String),
	    _offset UInt64,
	    _idx UInt32,
	    _rest String CODEC(ZSTD)
	)
	ENGINE = Memory;
	CREATE TABLE IF NOT EXISTS mdb.mysql_general
	(
	    timestamp DateTime CODEC(DoubleDelta, LZ4),
	    ms UInt16 DEFAULT 0,
	    cluster LowCardinality(String),
	    hostname LowCardinality(String),
	    origin LowCardinality(String),
	    id String,
	    command String,
	    argument String,
	    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
	    log_format LowCardinality(String),
	    datetime DateTime CODEC(DoubleDelta, LZ4),
	    raw String CODEC(ZSTD),
	    _timestamp DateTime CODEC(DoubleDelta, LZ4),
	    _partition LowCardinality(String),
	    _offset UInt64,
	    _idx UInt32,
	    _rest String CODEC(ZSTD)
	)
	ENGINE = Memory;
	CREATE TABLE IF NOT EXISTS mdb.mysql_slow_query
	(
	    timestamp DateTime CODEC(DoubleDelta, LZ4),
	    ms UInt16 DEFAULT 0,
	    cluster LowCardinality(String),
	    hostname LowCardinality(String),
	    origin LowCardinality(String),
	    id String,
	    user String,
	    schema String,
	    query String,
	    last_errno UInt32,
	    killed UInt32,
	    query_time Float64,
	    lock_time Float64,
	    rows_sent UInt32,
	    rows_examined UInt32,
	    rows_affected UInt32,
	    bytes_sent UInt64,
	    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
	    log_format LowCardinality(String),
	    datetime DateTime CODEC(DoubleDelta, LZ4),
	    raw String CODEC(ZSTD),
	    _timestamp DateTime CODEC(DoubleDelta, LZ4),
	    _partition LowCardinality(String),
	    _offset UInt64,
	    _idx UInt32,
	    _rest String CODEC(ZSTD)
	)
	ENGINE = Memory;
	CREATE TABLE IF NOT EXISTS mdb.mysql_audit
	(
	    cluster LowCardinality(String),
	    command_class String,
	    connection_id UInt32,
	    connection_type String,
	    db String,
	    hostname LowCardinality(String),
	    origin LowCardinality(String),
	    ip String,
	    timestamp DateTime CODEC(DoubleDelta, LZ4),
	    ms UInt16 DEFAULT 0,
	    mysql_version String,
	    name String,
	    os_login LowCardinality(String),
	    os_user LowCardinality(String),
	    os_version String,
	    priv_user String,
	    proxy_user String,
	    record String,
	    server_id String,
	    sqltext String,
	    startup_optionsi String,
	    status UInt32,
	    status_code UInt32,
	    user String,
	    version LowCardinality(String),
	    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
	    log_format LowCardinality(String),
	    datetime DateTime CODEC(DoubleDelta, LZ4),
	    raw String CODEC(ZSTD),
	    _timestamp DateTime CODEC(DoubleDelta, LZ4),
	    _partition LowCardinality(String),
	    _offset UInt64,
	    _idx UInt32,
	    _rest String CODEC(ZSTD)
	)
	ENGINE = Memory;
	CREATE TABLE IF NOT EXISTS mdb.elasticsearch
	(
	    timestamp DateTime CODEC(DoubleDelta, LZ4),
	    ms UInt16 DEFAULT 0,
	    cluster LowCardinality(String),
	    hostname LowCardinality(String),
	    origin LowCardinality(String),
	    level LowCardinality(String),
	    component LowCardinality(String),
	    message String,
	    stacktrace String,
	    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
	    log_format LowCardinality(String),
	    datetime DateTime CODEC(DoubleDelta, LZ4),
	    _timestamp DateTime CODEC(DoubleDelta, LZ4),
	    _partition LowCardinality(String),
	    _offset UInt64,
	    _idx UInt32,
	    _rest String CODEC(ZSTD)
	)
	ENGINE = Memory;
	CREATE TABLE IF NOT EXISTS mdb.kibana
	(
	    timestamp DateTime CODEC(DoubleDelta, LZ4),
	    ms UInt16 DEFAULT 0,
	    cluster LowCardinality(String),
	    hostname LowCardinality(String),
	    origin LowCardinality(String),
	    type LowCardinality(String),
	    message String,
	    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
	    log_format LowCardinality(String),
	    datetime DateTime CODEC(DoubleDelta, LZ4),
	    _timestamp DateTime CODEC(DoubleDelta, LZ4),
	    _partition LowCardinality(String),
	    _offset UInt64,
	    _idx UInt32,
	    _rest String CODEC(ZSTD)
	)
	ENGINE = Memory;
	CREATE TABLE IF NOT EXISTS mdb.kafka
	(
		datetime Nullable(DateTime),
		log_format LowCardinality(String),
		timestamp Nullable(DateTime),
		message LowCardinality(String),
		origin LowCardinality(String),
		severity LowCardinality(String),
		cluster LowCardinality(String),
		hostname LowCardinality(String),
	    ms UInt16 DEFAULT 0,
		_rest Nullable(String),
		_timestamp DateTime,
		_partition String,
		_offset UInt64,
		_idx UInt32
	)
	ENGINE = Memory;
	CREATE TABLE IF NOT EXISTS mdb.greenplum
	(
		timestamp DateTime,
		ms UInt32,
		cluster LowCardinality(String),
		hostname LowCardinality(String),
		origin LowCardinality(String),
		user_name String,
		database_name String,
		process_id String,
		session_start_time DateTime,
		transaction_id UInt64,
		sql_state_code LowCardinality(String),
		internal_query String,
		internal_query_pos UInt32,
		log_format LowCardinality(String),
		insert_time DateTime,
		sub_tranx_id String,
		remote_host String,
		file_name String,
		distr_tranx_id String,
		event_severity String,
		event_hint String,
		event_time DateTime,
		debug_query_string String,
		gp_session_id String,
		stack_trace String,
		local_tranx_id String,
		error_cursor_pos UInt32,
		gp_command_count String,
		slice_id String,
		event_detail String,
		event_context String,
		event_message String,
		func_name String,
		remote_port String,
		thread_id String,
		file_line UInt32,
		gp_segment String,
		gp_host_type String,
		gp_preferred_role String,
		_timestamp DateTime,
		_partition LowCardinality(String),
		_offset UInt64,
		_idx UInt32,
		_rest String
	)
	ENGINE = Memory;
	CREATE TABLE IF NOT EXISTS mdb.greenplum_odyssey
	(
		timestamp DateTime,
		ms UInt32,
		cluster LowCardinality(String),
		hostname LowCardinality(String),
		origin LowCardinality(String),
		text String,
		client_id String,
		server_id String,
		context String,
		db String,
		user String,
		pid UInt32,
		level String,
		insert_time DateTime,
		log_format LowCardinality(String),
		_timestamp DateTime,
		_partition LowCardinality(String),
		_offset UInt64,
		_idx UInt32,
		_rest String
	)
	ENGINE = Memory
`
