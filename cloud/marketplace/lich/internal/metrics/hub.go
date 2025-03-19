package metrics

import (
	"sync"

	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

type Hub struct {
	registry *solomon.Registry

	handlersMetrics handlersMetrics

	mu sync.Mutex
}

func NewHub() *Hub {
	return &Hub{
		registry: solomon.NewRegistry(
			solomon.NewRegistryOpts().SetPrefix("license-check"),
		),

		handlersMetrics: make(handlersMetrics),
	}
}

func (h *Hub) Registry() *solomon.Registry {
	return h.registry
}
