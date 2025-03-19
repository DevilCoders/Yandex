package server

import (
	"io/ioutil"
	"net/http"
	"os"
	"strings"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"go.uber.org/zap"
	"go.uber.org/zap/zaptest"
	"golang.org/x/net/context"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/internal/dockerprocess"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/library/go/test/yatest"
)

const defaultDatabase = "default"

func TestStartMonitoring(t *testing.T) {
	a := assert.New(t)
	ctx := ctxlog.WithLogger(context.Background(), zaptest.NewLogger(t).WithOptions(zap.Development()))
	defaultConfig := os.Getenv("TEST_CONFIG_PATH")
	if defaultConfig == "" {
		defaultConfig = "cloud/compute/snapshot/config.toml"
	}
	defaultConfig = yatest.SourcePath(defaultConfig)

	err := config.LoadConfig(defaultConfig)
	a.NoError(err)

	conf, err := config.GetConfig()
	a.NoError(err)

	endpointBytes, err := ioutil.ReadFile("ydb_endpoint.txt")
	if err != nil {
		panic(err)
	}
	ydbEndpoint := strings.TrimSpace(string(endpointBytes))
	ctxlog.G(ctx).Info("YDB endpoint", zap.ByteString("endpoint", endpointBytes))

	rootBytes, err := ioutil.ReadFile("ydb_database.txt")
	if err != nil {
		panic(err)
	}
	ydbRoot := strings.TrimSpace(string(rootBytes))
	if !strings.HasPrefix(ydbRoot, "/") {
		ydbRoot = "/" + ydbRoot
	}
	ctxlog.G(ctx).Info("YDB root", zap.String("root", ydbRoot))

	defaultDB := conf.Kikimr[defaultDatabase]
	defaultDB.DBHost = ydbEndpoint
	defaultDB.Root = ydbRoot
	defaultDB.DBName = ydbRoot
	conf.Kikimr[defaultDatabase] = defaultDB

	dockerprocess.SetDockerConfig(conf.Nbd.DockerConfig)

	go func() {
		err := Start(ctx, conf)
		a.NoError(err)
	}()
	time.Sleep(time.Second)
	resp, err := http.Get("http://" + conf.DebugServer.HTTPEndpoint.Addr + "/status")
	a.NoError(err)
	data, err := ioutil.ReadAll(resp.Body)
	a.NoError(err)
	a.True(strings.Contains(string(data), `"nbd":"Ok"`))
}
