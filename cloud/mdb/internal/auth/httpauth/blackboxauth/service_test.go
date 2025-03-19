package blackboxauth

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/mdb/internal/auth/tvm/tvmtool"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func Test_ServiceNewFromConfig(t *testing.T) {
	tests := []struct {
		name   string
		config Config
	}{
		{
			name: "blackbox & tvm only",
			config: Config{
				Tvm: tvmtool.Config{
					Alias: "mdb-deploy-api",
					Token: "",
					URI:   "http://localhost:50001",
				},
				BlackboxURI:   "http://loclahost:8080",
				BlackboxAlias: "blackbox",
			},
		},
		{
			name: "cache",
			config: Config{
				Tvm: tvmtool.Config{
					Alias: "mdb-deploy-api",
					Token: "",
					URI:   "http://localhost:50001",
				},
				BlackboxURI:   "http://loclahost:8080",
				BlackboxAlias: "blackbox",
				CacheTTL:      time.Duration(1) * time.Second,
				CacheSize:     100,
			},
		},
		{
			name: "scopes checker",
			config: Config{
				Tvm: tvmtool.Config{
					Alias: "mdb-deploy-api",
					Token: "",
					URI:   "http://localhost:50001",
				},
				BlackboxURI:   "http://loclahost:8080",
				BlackboxAlias: "blackbox",
				UserScopes:    []string{"test:write"},
			},
		},
		{
			name: "login checker",
			config: Config{
				Tvm: tvmtool.Config{
					Alias: "mdb-deploy-api",
					Token: "",
					URI:   "http://localhost:50001",
				},
				BlackboxURI:    "http://loclahost:8080",
				BlackboxAlias:  "blackbox",
				LoginWhiteList: []string{"testuser"},
			},
		},
		{
			name: "all",
			config: Config{
				Tvm: tvmtool.Config{
					Alias: "mdb-deploy-api",
					Token: "",
					URI:   "http://localhost:50001",
				},
				BlackboxURI:    "http://loclahost:8080",
				BlackboxAlias:  "blackbox",
				LoginWhiteList: []string{"testuser"},
				UserScopes:     []string{"test:write"},
				CacheTTL:       time.Duration(1) * time.Second,
				CacheSize:      100,
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			_, err := NewFromConfig(tt.config, &nop.Logger{})
			assert.NoError(t, err)
		})
	}
}
