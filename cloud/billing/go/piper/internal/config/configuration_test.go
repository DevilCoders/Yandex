package config

import (
	"errors"
	"fmt"
	"io/ioutil"
	"os"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/cfgtypes"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/timetool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/features"
	"a.yandex-team.ru/library/go/core/log"
)

type configurationTestSuite struct {
	baseTestSuite
}

func TestConfiguration(t *testing.T) {
	suite.Run(t, new(configurationTestSuite))
}

func (suite *configurationTestSuite) TestLockboxLoadYaml() {
	suite.yamlConfig.SetData(`
status_server:
  enabled: true
  port: 999
`)
	suite.container.testLockboxConfigurator(testConfigurator{
		yamlGetFunc: yamlGetFunc(func() ([]byte, error) {
			return []byte(`{"status_server":{"port": 101}}`), nil
		}),
	})

	want := cfgtypes.StatusServerConfig{
		Enabled: cfgtypes.BoolTrue,
		Port:    101,
	}

	got, err := suite.container.GetStatusServerConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestLockboxLoadConfig() {
	suite.yamlConfig.SetData(`
status_server:
  enabled: true
  port: 999
`)
	suite.container.testLockboxConfigurator(testConfigurator{
		configuratorFunc: configuratorFunc(func(namespace string) (map[string]string, error) {
			if namespace != "piper" {
				return nil, errors.New("unknown namespace")
			}
			return map[string]string{
				"status_server.port": "102",
			}, nil
		}),
	})

	want := cfgtypes.StatusServerConfig{
		Enabled: cfgtypes.BoolTrue,
		Port:    102,
	}

	got, err := suite.container.GetStatusServerConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestLockboxLoadAll() {
	suite.yamlConfig.SetData(`
status_server:
  enabled: true
  port: 999
`)
	suite.container.testLockboxConfigurator(testConfigurator{
		yamlGetFunc: yamlGetFunc(func() ([]byte, error) {
			return []byte(`{"status_server":{"port": 101}}`), nil
		}),
		configuratorFunc: configuratorFunc(func(namespace string) (map[string]string, error) {
			if namespace != "piper" {
				return nil, errors.New("unknown namespace")
			}
			return map[string]string{
				"status_server.port": "102",
			}, nil
		}),
	})

	want := cfgtypes.StatusServerConfig{
		Enabled: cfgtypes.BoolTrue,
		Port:    102,
	}

	got, err := suite.container.GetStatusServerConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestStatusServerConfig() {
	suite.yamlConfig.SetData(`
status_server:
  enabled: true
  port: 999
`)
	want := cfgtypes.StatusServerConfig{
		Enabled: cfgtypes.BoolTrue,
		Port:    999,
	}

	got, err := suite.container.GetStatusServerConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestLogConfig() {
	suite.yamlConfig.SetData(`
log:
  level: info
  paths:
  - file
  - stdout
  - logbroker://localhost:1
  enable_journald: true
`)
	want := cfgtypes.LoggingConf{
		Level:          log.InfoLevel,
		Paths:          []string{"file", "stdout", "logbroker://localhost:1"},
		EnableJournald: cfgtypes.BoolTrue,
	}

	got, err := suite.container.GetLoggingConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestTracingConfig() {
	suite.yamlConfig.SetData(`
trace:
  enabled: true
  queue_size: 10
  local_agent_hostport: localhost:1
`)
	want := cfgtypes.TraceConf{
		Enabled:            cfgtypes.BoolTrue,
		QueueSize:          10,
		LocalAgentHostPort: "localhost:1",
	}

	got, err := suite.container.GetTracingConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestFeaturesConfig() {
	suite.yamlConfig.SetData(`
features:
  tz: UTC
  drop_duplicates: true
`)
	want := cfgtypes.FeaturesConfig{
		Tz:             "UTC",
		DropDuplicates: cfgtypes.BoolTrue,
	}

	got, err := suite.container.GetFeaturesConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)

	flag := features.Default()

	suite.EqualValues(true, flag.DropDuplicates())
	suite.EqualValues(*timetool.ForceParseTz("UTC"), *flag.LocalTimezone())
}

func (suite *configurationTestSuite) TestTLSConfig() {
	suite.yamlConfig.SetData(`
tls:
  ca_path: /path/to/ca/pem
`)
	want := cfgtypes.TLSConfig{
		CAPath: "/path/to/ca/pem",
	}

	got, err := suite.container.GetTLSConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestUAConfig() {
	suite.yamlConfig.SetData(`
unified_agent:
  solomon_metrics_port: 22132
  health_check_port: 16301
`)
	want := cfgtypes.UAConfig{
		SolomonMetricsPort: 22132,
		HealthCheckPort:    16301,
	}

	got, err := suite.container.GetUAConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestYDBConfig() {
	suite.yamlConfig.SetData(`
ydb:
  address: local-ydb:1234
  database: billing-db
  disable_tls: true
  auth:
    type: static
    tvm_destination: 222111
    token: my-secret-token
    token_file: /path/to/token
  connect_timeout: 3
  request_timeout: 4
  discovery_interval: 5
  conn_max_lifetime: 6
  max_connections: 1
  max_idle_connections: 99
  max_direct_connections: 999
`)
	wantParams := cfgtypes.YDBParams{
		Address:    "local-ydb:1234",
		Database:   "billing-db",
		DisableTLS: cfgtypes.BoolTrue,
		Auth: cfgtypes.AuthConfig{
			Type:           cfgtypes.StaticTokenAuthType,
			TVMDestination: 222111,
			Token:          "my-secret-token",
			TokenFile:      "/path/to/token",
		},
		ConnectTimeout:       cfgtypes.Seconds(time.Second * 3),
		RequestTimeout:       cfgtypes.Seconds(time.Second * 4),
		DiscoveryInterval:    cfgtypes.Seconds(time.Second * 5),
		MaxConnections:       1,
		MaxIdleConnections:   99,
		MaxDirectConnections: 999,
		ConnMaxLifetime:      cfgtypes.Seconds(time.Second * 6),
	}
	want := cfgtypes.YDBConfig{
		YDBParams: wantParams,
		Installations: cfgtypes.YDBInstallations{
			Uniq:       wantParams,
			Cumulative: wantParams,
		},
	}

	got, err := suite.container.GetYDBConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestYDBUniqConfig() {
	suite.yamlConfig.SetData(`
ydb:
  installations:
    uniq:
      address: local-ydb:1234
      database: billing-db
      disable_tls: true
      auth:
        type: static
        tvm_destination: 222111
        token: my-secret-token
        token_file: /path/to/token
      connect_timeout: 3
      request_timeout: 4
      discovery_interval: 5
      conn_max_lifetime: 6
      max_connections: 1
      max_idle_connections: 99
      max_direct_connections: 999
`)
	wantParams := cfgtypes.YDBParams{
		Address:    "local-ydb:1234",
		Database:   "billing-db",
		DisableTLS: cfgtypes.BoolTrue,
		Auth: cfgtypes.AuthConfig{
			Type:           cfgtypes.StaticTokenAuthType,
			TVMDestination: 222111,
			Token:          "my-secret-token",
			TokenFile:      "/path/to/token",
		},
		ConnectTimeout:       cfgtypes.Seconds(time.Second * 3),
		RequestTimeout:       cfgtypes.Seconds(time.Second * 4),
		DiscoveryInterval:    cfgtypes.Seconds(time.Second * 5),
		MaxConnections:       1,
		MaxIdleConnections:   99,
		MaxDirectConnections: 999,
		ConnMaxLifetime:      cfgtypes.Seconds(time.Second * 6),
	}
	want := cfgtypes.YDBConfig{
		YDBParams: defaultLocalConfig.YDB.YDBParams,
		Installations: cfgtypes.YDBInstallations{
			Uniq:       wantParams,
			Cumulative: defaultLocalConfig.YDB.YDBParams,
		},
	}

	got, err := suite.container.GetYDBConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestYDBCumulativeConfig() {
	suite.yamlConfig.SetData(`
ydb:
  installations:
    cumulative:
      address: local-ydb:1234
      database: billing-db
      disable_tls: true
      auth:
        type: static
        tvm_destination: 222111
        token: my-secret-token
        token_file: /path/to/token
      connect_timeout: 3
      request_timeout: 4
      discovery_interval: 5
      conn_max_lifetime: 6
      max_connections: 1
      max_idle_connections: 99
      max_direct_connections: 999
`)
	wantParams := cfgtypes.YDBParams{
		Address:    "local-ydb:1234",
		Database:   "billing-db",
		DisableTLS: cfgtypes.BoolTrue,
		Auth: cfgtypes.AuthConfig{
			Type:           cfgtypes.StaticTokenAuthType,
			TVMDestination: 222111,
			Token:          "my-secret-token",
			TokenFile:      "/path/to/token",
		},
		ConnectTimeout:       cfgtypes.Seconds(time.Second * 3),
		RequestTimeout:       cfgtypes.Seconds(time.Second * 4),
		DiscoveryInterval:    cfgtypes.Seconds(time.Second * 5),
		MaxConnections:       1,
		MaxIdleConnections:   99,
		MaxDirectConnections: 999,
		ConnMaxLifetime:      cfgtypes.Seconds(time.Second * 6),
	}
	want := cfgtypes.YDBConfig{
		YDBParams: defaultLocalConfig.YDB.YDBParams,
		Installations: cfgtypes.YDBInstallations{
			Uniq:       defaultLocalConfig.YDB.YDBParams,
			Cumulative: wantParams,
		},
	}

	got, err := suite.container.GetYDBConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestCHConfig() {
	suite.yamlConfig.SetData(`
clickhouse:
  database: billing-db
  port: 9000
  auth:
    user: ch-user
    password: secret
  disable_tls: true
  max_connections: 8
  max_idle_connections: 99
  conn_max_lifetime: 1m
  shards:
    - hosts:
      - host1
      - "host2"
    - hosts: ["host3", "host4"]
`)
	want := cfgtypes.Clickhouse{
		ClickhouseCommonConfig: cfgtypes.ClickhouseCommonConfig{
			Database:   "billing-db",
			DisableTLS: cfgtypes.BoolTrue,
			Port:       9000,
			Auth: cfgtypes.ClickhouseAuth{
				User:     "ch-user",
				Password: "secret",
			},
			MaxConnections:     8,
			MaxIdleConnections: 99,
			ConnMaxLifetime:    cfgtypes.Seconds(1 * time.Minute),
		},
		Shards: []cfgtypes.ClickhouseShard{
			{Hosts: []string{"host1", "host2"}},
			{Hosts: []string{"host3", "host4"}},
		},
	}
	got, err := suite.container.GetClickhouseConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestIAMMetaConfig() {
	suite.yamlConfig.SetData(`
iam_meta:
  use_localhost: true
`)
	want := cfgtypes.IAMMetaConfig{
		UseLocalhost: cfgtypes.BoolTrue,
	}

	got, err := suite.container.GetIAMMetaConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestJWTConfig() {
	suite.yamlConfig.SetData(`
jwt:
  endpoint: iam-ticket-service-endpoint
  key_file: /path/to/key
  key: {"service_account_id":"sa-id","id":"key-id","private_key":"my-private-key"}
  audience: https://iam.api.cloud.yandex.net/iam/v1/tokens
`)
	want := cfgtypes.JWTConfig{
		Endpoint: "iam-ticket-service-endpoint",
		KeyFile:  "/path/to/key",
		Key: cfgtypes.JWTKey{
			ServiceAccountID: "sa-id",
			ID:               "key-id",
			PrivateKey:       "my-private-key",
		},
		Audience: "https://iam.api.cloud.yandex.net/iam/v1/tokens",
	}
	got, err := suite.container.GetJWTConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestJWTKeyFileConfig() {

	key := []byte(`
	{
	   "id": "ajebbgri9hf3u0jpsn4p",
	   "service_account_id": "ajeepikcaefrmjb96n6a",
	   "created_at": "2022-03-18T14:52:03.856691045Z",
	   "private_key": "-----BEGIN PRIVATE KEY-----"
	}`)

	// Create temporary key json
	file, _ := ioutil.TempFile(".", "key-*.json")
	_ = ioutil.WriteFile(file.Name(), key, 0600)
	defer func(path string) {
		err := os.Remove(path)
		if err != nil {
			suite.NoError(err)
		}
	}(file.Name())

	// Setup local config
	suite.yamlConfig.SetData(fmt.Sprintf(`
	jwt:
	  endpoint: iam-ticket-service-endpoint
	  key_file: %s
	  audience: https://iam.api.cloud.yandex.net/iam/v1/tokens
	`, file.Name()),
	)
	want := cfgtypes.JWTKey{
		ServiceAccountID: "ajeepikcaefrmjb96n6a",
		ID:               "ajebbgri9hf3u0jpsn4p",
		PrivateKey:       "-----BEGIN PRIVATE KEY-----",
	}
	got, err := suite.container.GetJWTKeyConfig(file.Name())
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestTVMConfig() {
	suite.yamlConfig.SetData(`
tvm:
  client_id: 12345
  api_url: localhost:9999
`)
	want := cfgtypes.TVMConfig{
		ClientID: "12345",
		APIURL:   "localhost:9999",
	}

	got, err := suite.container.GetTVMConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestRMConfig() {
	suite.yamlConfig.SetData(`
resource_manager:
  endpoint: rm.iam.endpoint
  auth:
    type: static
    tvm_destination: 222111
    token: my-secret-token
    token_file: /path/to/token
  grpc:
    request_retries: 100
    request_retry_timeout: 1
    keepalive_time: 1m
    keepalive_timeout: 1
`)
	want := cfgtypes.RMConfig{
		Endpoint: "rm.iam.endpoint",
		Auth: cfgtypes.AuthConfig{
			Type:           cfgtypes.StaticTokenAuthType,
			TVMDestination: 222111,
			Token:          "my-secret-token",
			TokenFile:      "/path/to/token",
		},
		GRPC: cfgtypes.GRPCConfig{
			RequestRetries:      100,
			RequestRetryTimeout: cfgtypes.Seconds(time.Second),
			KeepAliveTime:       cfgtypes.Seconds(time.Minute),
			KeepAliveTimeout:    cfgtypes.Seconds(time.Second),
		},
	}

	got, err := suite.container.GetRMConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestRMConfigOverride() {
	suite.yamlConfig.SetData(`
resource_manager:
  endpoint: rm.iam.endpoint
  auth:
    type: static
    tvm_destination: 222111
    token: my-secret-token
    token_file: /path/to/token
  grpc:
    request_retries: 100
    request_retry_timeout: 1
    keepalive_time: 1m
    keepalive_timeout: 1
`)
	suite.container.testDBConfigurator(func(namespace string) (map[string]string, error) {
		if namespace != "piper" {
			return nil, errors.New("unknown namespace")
		}
		return map[string]string{
			"resource_manager.endpoint":                   "rm.iam.db.endpoint",
			"resource_manager.auth.type":                  "iam-metadata",
			"resource_manager.auth.tvm_destination":       "333444",
			"resource_manager.auth.token":                 "db-token",
			"resource_manager.auth.token_file":            "/db/path",
			"resource_manager.grpc.request_retries":       "42",
			"resource_manager.grpc.request_retry_timeout": "1",
			"resource_manager.grpc.keepalive_time":        "2",
			"resource_manager.grpc.keepalive_timeout":     "3",
		}, nil
	})

	want := cfgtypes.RMConfig{
		Endpoint: "rm.iam.db.endpoint",
		Auth: cfgtypes.AuthConfig{
			Type:           cfgtypes.IAMMetaAuthType,
			TVMDestination: 333444,
			Token:          "db-token",
			TokenFile:      "/db/path",
		},
		GRPC: cfgtypes.GRPCConfig{
			RequestRetries:      42,
			RequestRetryTimeout: cfgtypes.Seconds(time.Second * 1),
			KeepAliveTime:       cfgtypes.Seconds(time.Second * 2),
			KeepAliveTimeout:    cfgtypes.Seconds(time.Second * 3),
		},
	}

	got, err := suite.container.GetRMConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestLogbrokerConfig() {
	suite.yamlConfig.SetData(`
logbroker_installations:
  yc_preprod:
    host: original.logbroker.host
    port: 2135
    database: /pre-prod_global/aoeb66ftj1tbt1b2eimn/cc8035oc71oh9um52mv3
    auth:
      type: iam-metadata
`)

	want := cfgtypes.LbInstallationConfig{
		Host:     "original.logbroker.host",
		Port:     2135,
		Database: "/pre-prod_global/aoeb66ftj1tbt1b2eimn/cc8035oc71oh9um52mv3",
		Auth:     cfgtypes.AuthConfig{Type: cfgtypes.IAMMetaAuthType},
	}

	got, err := suite.container.GetLbInstallationConfig("yc_preprod")
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestLogbrokerConfigOverride() {
	suite.yamlConfig.SetData(`
logbroker_installations:
  yc_preprod:
    host: original.logbroker.host
    port: 2135
    database: /pre-prod_global/aoeb66ftj1tbt1b2eimn/cc8035oc71oh9um52mv3
    auth:
      type: iam-metadata
`)
	suite.container.testLockboxConfigurator(testConfigurator{
		configuratorFunc: configuratorFunc(func(namespace string) (map[string]string, error) {
			if namespace != "piper" {
				return nil, errors.New("unknown namespace")
			}
			return map[string]string{
				"logbroker_installations.yc_preprod.host": "redirected",
			}, nil
		}),
	})

	want := cfgtypes.LbInstallationConfig{
		Host:     "redirected",
		Port:     2135,
		Database: "/pre-prod_global/aoeb66ftj1tbt1b2eimn/cc8035oc71oh9um52mv3",
		Auth:     cfgtypes.AuthConfig{Type: cfgtypes.IAMMetaAuthType},
	}

	got, err := suite.container.GetLbInstallationConfig("yc_preprod")
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestResharderConfig() {
	suite.yamlConfig.SetData(`
resharder:
  metrics_grace: 1h
  sink:
    logbroker:
      enabled: true
      installation: test
      topic: /test/topic
      partitions: 42
    logbroker_errors:
      enabled: true
      installation: test
      topic: /test/errors
    ydb_errors:
      enabled: true
  source:
    /test/source:
      logbroker:
        installation: test
        consumer: /piper/consumer
        topic: /test/source
        max_messages: 1000
        max_size: 50MiB
        lag: 288h
        batch_limit: 1
        batch_size: 30 MiB
        batch_timeout: 15
      handler: test
    /defaults:
      logbroker:
        installation: test
        consumer: /default/consumer
        topic: /test/default
      handler: test
`)

	want := cfgtypes.ResharderConfig{
		MetricsGrace: cfgtypes.Seconds(time.Hour * 1),
		Sink: cfgtypes.ResharderSinkConfig{
			Logbroker: cfgtypes.LogbrokerSinkConfig{
				Enabled:      cfgtypes.BoolTrue,
				Installation: "test",
				Topic:        "/test/topic",
				Partitions:   42,
				Route:        "source_metrics",
				MaxParallel:  32,
				SplitSize:    50 * cfgtypes.MiB,
			},
			LogbrokerErrors: cfgtypes.LogbrokerSinkConfig{
				Enabled:      cfgtypes.BoolTrue,
				Installation: "test",
				Topic:        "/test/errors",
				Route:        "invalid_metrics",
				MaxParallel:  32,
				SplitSize:    50 * cfgtypes.MiB,
			},
			YDBErrors: cfgtypes.YDBSinkConfig{
				Enabled: cfgtypes.BoolTrue,
			},
		},
		Source: map[string]*cfgtypes.ResharderSourceConfig{
			"/test/source": {
				Logbroker: cfgtypes.LogbrokerSourceConfig{
					Installation: "test",
					Consumer:     "/piper/consumer",
					Topic:        "/test/source",
					MaxMessages:  1000,
					MaxSize:      50 * cfgtypes.MiB,
					Lag:          cfgtypes.Seconds(12 * 24 * time.Hour),
					BatchLimit:   1,
					BatchSize:    30 * cfgtypes.MiB,
					BatchTimeout: cfgtypes.Seconds(15 * time.Second),
				},
				Handler:  "test",
				Parallel: 1,
				Params: cfgtypes.ResharderHandlerConfig{
					ChunkSize:      52428800,
					MetricLifetime: cfgtypes.Seconds(288 * time.Hour),
					MetricGrace:    cfgtypes.Seconds(9 * time.Hour),
				},
			},
			"/defaults": {
				Logbroker: cfgtypes.LogbrokerSourceConfig{
					Installation: "test",
					Consumer:     "/default/consumer",
					Topic:        "/test/default",
					MaxMessages:  1000,
					MaxSize:      cfgtypes.MiB,
					Lag:          cfgtypes.Seconds(12 * 24 * time.Hour),
					BatchLimit:   5000,
					BatchSize:    2 * cfgtypes.MiB,
					BatchTimeout: cfgtypes.Seconds(15 * time.Second),
				},
				Handler:  "test",
				Parallel: 1,
				Params: cfgtypes.ResharderHandlerConfig{
					ChunkSize:      52428800,
					MetricLifetime: cfgtypes.Seconds(288 * time.Hour),
					MetricGrace:    cfgtypes.Seconds(9 * time.Hour),
				},
			},
		},
	}

	got, err := suite.container.GetResharderConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestResharderConfigOverride() {
	suite.yamlConfig.SetData(`
resharder:
  source:
    /test/source:
      logbroker:
        installation: test
        consumer: /piper/consumer
        topic: /test/source
        max_messages: 1000
        max_size: 50MiB
        lag: 288h
        batch_limit: 1
        batch_size: 50MiB
        batch_timeout: 15
      handler: test
`)

	suite.container.testDBConfigurator(func(namespace string) (map[string]string, error) {
		if namespace != "piper" {
			return nil, errors.New("unknown namespace")
		}
		return map[string]string{
			"resharder.source./test/source.logbroker.topic": "/test/other",
		}, nil
	})

	want := cfgtypes.ResharderConfig{
		Source: map[string]*cfgtypes.ResharderSourceConfig{
			"/test/source": {
				Logbroker: cfgtypes.LogbrokerSourceConfig{
					Installation: "test",
					Consumer:     "/piper/consumer",
					Topic:        "/test/other",
					MaxMessages:  1000,
					MaxSize:      50 * cfgtypes.MiB,
					Lag:          cfgtypes.Seconds(12 * 24 * time.Hour),
					BatchLimit:   1,
					BatchSize:    50 * cfgtypes.MiB,
					BatchTimeout: cfgtypes.Seconds(15 * time.Second),
				},
				Handler:  "test",
				Parallel: 1,
				Params: cfgtypes.ResharderHandlerConfig{
					ChunkSize:      52428800,
					MetricLifetime: cfgtypes.Seconds(288 * time.Hour),
					MetricGrace:    cfgtypes.Seconds(9 * time.Hour),
				},
			},
		},
	}

	got, err := suite.container.GetResharderConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want.Source, got.Source)
}

func (suite *configurationTestSuite) TestDumperConfig() {
	suite.yamlConfig.SetData(`
dump:
  sink:
    ydb_errors:
      enabled: true
    ch_errors:
      enabled: true
  source:
    /test/errors:
      logbroker:
        installation: test
        consumer: /piper/errors-consumer
        topic: /test/errors
        max_messages: 1000
        max_size: 50MiB
        lag: 288h
        batch_limit: 100
        batch_timeout: 15
      handler: dumpErrors:ydb
    /test/cherrors:
      logbroker:
        installation: test
        consumer: /piper/clickhouse-errors-consumer
        topic: /test/errors
        max_messages: 1000
        max_size: 50MiB
        lag: 288h
        batch_limit: 100
        batch_timeout: 15
      handler: dumpErrors:clickhouse
`)

	want := cfgtypes.DumperConfig{
		Sink: cfgtypes.DumpSinkConfig{
			YDBErrors: cfgtypes.YDBSinkConfig{
				Enabled: cfgtypes.BoolTrue,
			},
			ClickhouseErrors: cfgtypes.ClickhouseSinkConfig{
				Enabled: cfgtypes.BoolTrue,
			},
		},
		Source: map[string]*cfgtypes.DumpSourceConfig{
			"/test/errors": {
				Logbroker: cfgtypes.LogbrokerSourceConfig{
					Installation: "test",
					Consumer:     "/piper/errors-consumer",
					Topic:        "/test/errors",
					MaxMessages:  1000,
					MaxSize:      50 * cfgtypes.MiB,
					Lag:          cfgtypes.Seconds(12 * 24 * time.Hour),
					BatchLimit:   100,
					BatchSize:    2 * cfgtypes.MiB,
					BatchTimeout: cfgtypes.Seconds(15 * time.Second),
				},
				Handler: "dumpErrors:ydb",
			},
			"/test/cherrors": {
				Logbroker: cfgtypes.LogbrokerSourceConfig{
					Installation: "test",
					Consumer:     "/piper/clickhouse-errors-consumer",
					Topic:        "/test/errors",
					MaxMessages:  1000,
					MaxSize:      50 * cfgtypes.MiB,
					Lag:          cfgtypes.Seconds(12 * 24 * time.Hour),
					BatchLimit:   100,
					BatchSize:    2 * cfgtypes.MiB,
					BatchTimeout: cfgtypes.Seconds(15 * time.Second),
				},
				Handler: "dumpErrors:clickhouse",
			},
		},
	}

	got, err := suite.container.GetDumperConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want, got)
}

func (suite *configurationTestSuite) TestDumperConfigOverride() {
	suite.yamlConfig.SetData(`
dump:
  sink:
    ydb_errors:
      enabled: true
    ch_errors:
      enabled: true
  source:
    /test/errors:
      logbroker:
        installation: test
        consumer: /piper/errors-consumer
        topic: /test/errors
        max_messages: 1000
        max_size: 50MiB
        lag: 288h
        batch_limit: 100
        batch_timeout: 15
      handler: dumpErrors:ydb
    /test/cherrors:
      logbroker:
        installation: test
        consumer: /piper/clickhouse-errors-consumer
        topic: /test/errors
        max_messages: 1000
        max_size: 50MiB
        lag: 288h
        batch_limit: 100
        batch_timeout: 15
      handler: dumpErrors:clickhouse
`)

	suite.container.testDBConfigurator(func(namespace string) (map[string]string, error) {
		if namespace != "piper" {
			return nil, errors.New("unknown namespace")
		}
		return map[string]string{
			"dump.source./test/errors.logbroker.installation":   "db.override",
			"dump.source./test/errors.logbroker.topic":          "/test/other",
			"dump.source./test/cherrors.logbroker.installation": "db.override",
			"dump.source./test/cherrors.logbroker.topic":        "/test/ch-separated",
		}, nil
	})

	want := cfgtypes.DumperConfig{
		Source: map[string]*cfgtypes.DumpSourceConfig{
			"/test/errors": {
				Logbroker: cfgtypes.LogbrokerSourceConfig{
					Installation: "db.override",
					Consumer:     "/piper/errors-consumer",
					Topic:        "/test/other",
					MaxMessages:  1000,
					MaxSize:      50 * cfgtypes.MiB,
					Lag:          cfgtypes.Seconds(12 * 24 * time.Hour),
					BatchLimit:   100,
					BatchSize:    2 * cfgtypes.MiB,
					BatchTimeout: cfgtypes.Seconds(15 * time.Second),
				},
				Handler: "dumpErrors:ydb",
			},
			"/test/cherrors": {
				Logbroker: cfgtypes.LogbrokerSourceConfig{
					Installation: "db.override",
					Consumer:     "/piper/clickhouse-errors-consumer",
					Topic:        "/test/ch-separated",
					MaxMessages:  1000,
					MaxSize:      50 * cfgtypes.MiB,
					Lag:          cfgtypes.Seconds(12 * 24 * time.Hour),
					BatchLimit:   100,
					BatchSize:    2 * cfgtypes.MiB,
					BatchTimeout: cfgtypes.Seconds(15 * time.Second),
				},
				Handler: "dumpErrors:clickhouse",
			},
		},
	}

	got, err := suite.container.GetDumperConfig()
	suite.Require().NoError(err)
	suite.EqualValues(want.Source, got.Source)
}
