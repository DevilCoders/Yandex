package health

import (
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
)

// LastEvent return RawEvent with greatest ReceivedTime
func LastEvent(events []juggler.RawEvent) juggler.RawEvent {
	var last juggler.RawEvent
	for _, e := range events {
		if e.ReceivedTime.After(last.ReceivedTime) {
			last = e
		}
	}
	return last
}

var defaultServices = []string{"META"}
var clusterTypeToServices = map[string][]string{
	"postgresql_cluster": {"META", "pg_ping"},
}

func ServicesByTags(tags tags.ClusterTags) []string {
	custom, ok := clusterTypeToServices[string(tags.Meta.Type)]
	if ok {
		return custom
	}
	return defaultServices
}

// ManagedByHealth return whenever that cluster managed by mdb-health
func ManagedByHealth(t tags.ClusterTags) bool {
	return len(t.Meta.Type) > 0
}
