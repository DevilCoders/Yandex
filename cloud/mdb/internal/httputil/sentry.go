package httputil

import (
	"net/http"

	"a.yandex-team.ru/cloud/mdb/internal/request"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
)

func gatherSentryTags(r *http.Request) map[string]string {
	if r == nil {
		return nil
	}
	tags := map[string]string{
		"method":      r.Method,
		"remote_addr": r.RemoteAddr,
		"user_agent":  r.UserAgent(),
	}
	if fwdFor := r.Header.Get("X-Forwarded-For"); fwdFor != "" {
		tags["x_forwarded_for"] = fwdFor
	}
	if fwdAgent := r.Header.Get("X-Forwarded-Agent"); fwdAgent != "" {
		tags["x_forwarded_agent"] = fwdAgent
	}
	if r.URL != nil {
		tags["request_uri"] = r.URL.RequestURI()
	}
	rTags := request.SentryTags(r.Context())
	for name, value := range rTags {
		tags[name] = value
	}
	return tags
}

// ReportErrorToSentry decide should that err be reported or not. If yes then sends it to Sentry
func ReportErrorToSentry(err error, r *http.Request) {
	if err == nil {
		return
	}
	if sentry.NeedReport(err) {
		sentry.GlobalClient().CaptureError(r.Context(), err, gatherSentryTags(r))
	}
}
