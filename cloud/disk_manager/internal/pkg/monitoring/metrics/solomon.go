package metrics

import (
	"fmt"
	"net/http"

	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/yandex/solomon/reporters/puller/httppuller"
)

////////////////////////////////////////////////////////////////////////////////

func CreateSolomonRegistry(
	mux *http.ServeMux,
	path string,
) Registry {

	registry := solomon.NewRegistry(solomon.NewRegistryOpts().SetRated(true))
	mux.Handle(fmt.Sprintf("/solomon/%v", path), httppuller.NewHandler(registry))

	return registry
}
