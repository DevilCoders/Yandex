package ualogs

import (
	"encoding/json"
	"fmt"
	"os"
	"path"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/test/yatest"
)

// From test receipt
var (
	portEnv = os.Getenv("UA_RECIPE_PORT")
	baseDir = func() string {
		if op := yatest.OutputRAMDrivePath(""); op != "" {
			return op
		}
		if op := yatest.OutputPath(""); op != "" {
			return op
		}
		return ""
	}()
	uaPort = func() string {
		if portEnv != "" {
			return portEnv
		}
		return "16392"
	}()
)

func init() {
	err := RegisterSink()
	if err != nil {
		panic(err)
	}
}

type logTestSuite struct {
	suite.Suite
}

func TestLog(t *testing.T) {
	suite.Run(t, new(logTestSuite))
}

func (suite *logTestSuite) TestLogging() {
	cfg := zap.JSONConfig(log.InfoLevel)
	cfg.OutputPaths = []string{fmt.Sprintf("logbroker://localhost:%s", uaPort)}
	logger := zap.Must(cfg)

	logger.Info("test message", log.String("test", "value"))

	err := logger.L.Core().Sync()
	suite.Require().NoError(err)

	outputPath := path.Join(baseDir, "ua_recipe_working_dir/data/output")

	data, err := os.ReadFile(outputPath)
	suite.Require().NoError(err)

	var msg struct {
		Level string
		Msg   string
		Test  string
	}
	err = json.Unmarshal(data, &msg)
	suite.Require().NoError(err)
	suite.Equal("INFO", msg.Level)
	suite.Equal("test message", msg.Msg)
	suite.Equal("value", msg.Test)
}
