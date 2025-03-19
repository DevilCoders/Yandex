package monitoring

import (
	"net/http"

	"a.yandex-team.ru/library/go/core/metrics/solomon"

	"a.yandex-team.ru/library/go/yandex/solomon/reporters/puller/httppuller"
)

func newMetricsHandler(registry *solomon.Registry) http.HandlerFunc {
	return httppuller.NewHandler(registry).ServeHTTP
}
