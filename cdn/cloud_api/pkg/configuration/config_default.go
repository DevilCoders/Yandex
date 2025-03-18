package configuration

import (
	"time"

	"a.yandex-team.ru/library/go/core/log"
)

func NewDefaultConfig() *Config {
	return &Config{
		LoggerConfig: LoggerConfig{
			Level:    log.InfoLevel,
			Encoding: "console",
			Path:     "stdout",
		},
		ServerConfig: ServerConfig{
			ShutdownTimeout:             15 * time.Second,
			HTTPPort:                    8080,
			GRPCPort:                    7070,
			ConfigHandlerEnabled:        true,
			PprofEnabled:                true,
			HTTPResponsePanicStacktrace: true,
			TLSConfig: TLSConfig{
				UseTLS:      false,
				TLSCertPath: "",
				TLSKeyPath:  "",
			},
		},
		AuthClientConfig: AuthClientConfig{
			EnableMock:            false,
			Endpoint:              "",
			UserAgent:             "Internal-CDN",
			ConnectionInitTimeout: 15 * time.Second,
			MaxRetries:            5,
			RetryTimeout:          2 * time.Second,
			KeepAliveTime:         10 * time.Second,
			KeepAliveTimeout:      1 * time.Second,
		},
		StorageConfig: StorageConfig{
			DSN:         "",
			ReplicaDSN:  "",
			MaxIdleTime: 0,
			MaxLifeTime: 0,
			MaxIdleConn: 2,
			MaxOpenConn: 0,
		},
		APIConfig: APIConfig{
			FolderIDCacheSize: 10_000,
			ConsoleAPIConfig: ConsoleAPIConfig{
				EnableAutoActivateEntities: true,
			},
			UserAPIConfig: UserAPIConfig{
				EnableAutoActivateEntities: false,
			},
		},
		DatabaseGCConfig: DatabaseGCConfig{
			EraseSoftDeleted: EraseSoftDeleted{
				Enabled:       false,
				TimeThreshold: 720 * time.Hour,
			},
			EraseOldVersions: EraseOldVersions{
				Enabled:          false,
				VersionThreshold: 10,
			},
		},
	}
}
