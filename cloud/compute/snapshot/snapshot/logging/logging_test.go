package logging

import (
	"errors"
	"testing"

	dto "github.com/prometheus/client_model/go"
	"github.com/stretchr/testify/assert"
	"go.uber.org/zap/zapcore"
	"golang.org/x/net/context"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

func TestErrorMessageHook(t *testing.T) {
	a := assert.New(t)
	conf := config.Config{
		Logging: config.LoggerConfig{
			Level:  zapcore.DebugLevel,
			Output: "stdout",
		},
	}
	logger, closer, err := SetupLogging(conf)
	a.NoError(err)
	defer closer()
	logger.Error("test error")
	ctx := ctxlog.WithLogger(context.Background(), logger)
	ctxlog.DebugErrorCtx(ctx, errors.New("test error"), "test error 2")
	data := dto.Metric{}
	err = misc.LogErrorMessages.Write(&data)
	a.NoError(err)
	a.Equal(float64(2), *data.Counter.Value)
}
