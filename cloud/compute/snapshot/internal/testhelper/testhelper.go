package testhelper

import (
	"context"
	"fmt"
	"io/ioutil"
	"math/rand"
	"os"
	"strings"
	"time"

	"go.uber.org/zap"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/logging"
	"a.yandex-team.ru/library/go/test/yatest"
)

const (
	defaultDatabase = "default"
	defaultNBSConf  = "default"
)

func InitConfig() {
	rand.Seed(time.Now().UnixNano())

	defaultConfig := os.Getenv("TEST_CONFIG_PATH")
	if defaultConfig == "" {
		defaultConfig = "config.toml"
	}
	defaultConfig = yatest.SourcePath(defaultConfig)
	if err := config.LoadConfig(defaultConfig); err != nil {
		panic(err)
	}

	logger := logging.SetupCliLogging()
	ctx := ctxlog.WithLogger(context.Background(), logger)

	conf, err := config.GetConfig()
	if err != nil {
		panic(err)
	}

	_ = os.Remove(conf.QemuDockerProxy.SocketPath)

	endpointBytes, err := ioutil.ReadFile("ydb_endpoint.txt")
	if err != nil {
		panic(err)
	}
	ydbEndpoint := strings.TrimSpace(string(endpointBytes))
	logger.Info("YDB endpoint", zap.ByteString("endpoint", endpointBytes))

	rootBytes, err := ioutil.ReadFile("ydb_database.txt")
	if err != nil {
		panic(err)
	}
	ydbRoot := strings.TrimSpace(string(rootBytes))
	if !strings.HasPrefix(ydbRoot, "/") {
		ydbRoot = "/" + ydbRoot
	}
	logger.Info("YDB root", zap.String("root", ydbRoot))

	defaultDB := conf.Kikimr[defaultDatabase]
	defaultDB.DBHost = ydbEndpoint
	defaultDB.Root = ydbRoot
	defaultDB.DBName = ydbRoot
	conf.Kikimr[defaultDatabase] = defaultDB
	defaultNBS := conf.Nbs[defaultNBSConf]
	port, _ := os.LookupEnv("LOCAL_KIKIMR_INSECURE_NBS_SERVER_PORT")
	logger.Debug("LOCAL_KIKIMR_INSECURE_NBS_SERVER_PORT", zap.String("port", port))
	if port != "" {
		defaultNBS.Hosts = []string{fmt.Sprintf("%s:%s", "localhost", port)}
	}
	conf.Nbs[defaultNBSConf] = defaultNBS
	st, err := lib.PrepareStorage(ctx, &conf, misc.TableOpCreate|misc.TableOpDrop)
	if err != nil {
		panic(err)
	}
	defer st.Close()
}
