package monitoring

import (
	"fmt"
	"net/http"

	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/monitoring/status"
)

const (
	statusOk = iota
	statusWarning
	statusError
	statusUndefined
)

func newStatusHandler(collector *status.StatusCollector) http.HandlerFunc {
	return func(rw http.ResponseWriter, r *http.Request) {
		scoppedLogger := ctxtools.Logger(r.Context())

		status := collector.OverviewStatus()
		code := codeFromStatus(status.Code)
		switch code {
		case statusUndefined:
			rw.WriteHeader(http.StatusInternalServerError)
			scoppedLogger.Error("received undefined status", log.String("status", status.Code.String()))
		default:
			rw.WriteHeader(http.StatusOK)
		}

		_, _ = fmt.Fprintf(rw, "%s:%d:%s", status.Name, code, status.Description)
		defer scoppedLogger.Info("processed overall status request", log.Int("code", code))
	}
}

func codeFromStatus(s status.StatusCode) (code int) {
	switch s {
	case status.StatusCodeOK:
		return statusOk
	case status.StatusCodeWarn:
		return statusOk
	case status.StatusCodeError:
		return statusOk
	default:
		return statusUndefined
	}
}
