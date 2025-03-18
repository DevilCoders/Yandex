package tracers

import (
	"fmt"

	"golang.yandex/hasql"

	"a.yandex-team.ru/library/go/core/log"
)

// Log returns tracer that logs all trace events
func Log(l log.Logger) hasql.Tracer {
	return hasql.Tracer{
		UpdateNodes: func() {
			l.Debug("updating cluster nodes")
		},
		UpdatedNodes: func(nodes hasql.AliveNodes) {
			l.Debug("updated cluster nodes", log.String("alive", fmt.Sprintf("%s", nodes)))
		},
		NodeDead: func(node hasql.Node, err error) {
			l.Debug("node is dead", log.String("node", node.String()), log.Error(err))
		},
		NodeAlive: func(node hasql.Node) {
			l.Debug("node is alive", log.String("node", node.String()))
		},
		NotifiedWaiters: func() {
			l.Debug("notified waiters")
		},
	}
}
