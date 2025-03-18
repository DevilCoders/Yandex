package recovery

import (
	"errors"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/go-resty/resty/v2"
	"github.com/stretchr/testify/assert"
	uzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"go.uber.org/zap/zaptest/observer"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func Test_wrap(t *testing.T) {
	logger, err := zap.New(zap.CLIConfig(log.DebugLevel))
	assert.NoError(t, err)

	core, logs := observer.New(uzap.DebugLevel)

	logger.L = logger.L.WithOptions(uzap.WrapCore(func(c zapcore.Core) zapcore.Core {
		return core
	}))

	mw := New(
		WithCallBack(func(w http.ResponseWriter, r *http.Request, _ error) {
			w.WriteHeader(http.StatusBadGateway)
		}),
		WithLogger(logger),
	)

	handler := http.HandlerFunc(func(_ http.ResponseWriter, r *http.Request) {
		panic(errors.New("put on a happy face"))
	})

	srv := httptest.NewServer(mw(handler))
	defer srv.Close()

	var resp *resty.Response
	assert.NotPanics(t, func() {
		resp, err = resty.New().
			SetBaseURL(srv.URL).
			R().
			Get("/")
	})

	assert.NoError(t, err)
	assert.Equal(t, http.StatusBadGateway, resp.StatusCode())

	loggedEntry := logs.AllUntimed()[0]
	assert.Equal(t, uzap.ErrorLevel, loggedEntry.Level)
	assert.Equal(t, "panic recovered", loggedEntry.Message)
	assert.Equal(t,
		map[string]interface{}{"error": "put on a happy face"},
		loggedEntry.ContextMap(),
	)
}
