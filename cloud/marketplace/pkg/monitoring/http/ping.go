package monitoring

import (
	"net/http"

	"a.yandex-team.ru/cloud/marketplace/pkg/monitoring/status"
)

type pingReport struct {
	Ping    string            `json:"ping"`
	Details map[string]string `json:"details"`
}

func newPingHandler(collector *status.StatusCollector) http.HandlerFunc {
	return func(rw http.ResponseWriter, r *http.Request) {
		report := newPingReport(collector.Statuses())
		sendJSONResponseSuccess(r.Context(), rw, report)
	}
}

func newPingReport(statuses []status.Status) (report pingReport) {
	var (
		hasWarning bool
		hasErrors  bool
	)

	report.Details = make(map[string]string)

	for _, s := range statuses {
		switch s.Code {
		case status.StatusCodeError:
			hasErrors = true
		case status.StatusCodeWarn:
			hasWarning = true
		}

		report.Details[s.Name] = s.Description
	}

	switch {
	case hasErrors:
		report.Ping = status.StatusCodeError.String()
	case hasWarning:
		report.Ping = status.StatusCodeWarn.String()
	default:
		report.Ping = status.StatusCodeOK.String()
	}

	return
}
