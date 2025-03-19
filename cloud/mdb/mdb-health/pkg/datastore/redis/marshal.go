package redis

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

func marshalFQDNKey(fqdn string) string {
	return fmt.Sprintf("fqdn:%s", fqdn)
}

func marshalTopologyKey(cid string) string {
	return fmt.Sprintf("topology:%s", cid)
}

func marshalClusterHealthKey(cid string) string {
	return fmt.Sprintf("health:%s", cid)
}

func marshalHostHealthKey(fqdn string) string {
	return fmt.Sprintf("health:%s", fqdn)
}

func marshalTopologyField(prefix topologyPrefix, sid string) string {
	return fmt.Sprintf("%s%s", prefix, sid)
}

func marshalAggregateKey(aggType types.AggType, sla, userfaultBroken bool) string {
	key := string(aggType)
	if sla {
		key = "sla_" + key
	}
	if userfaultBroken {
		key = "userfault_broken_" + key
	}
	return fmt.Sprintf("aggregate:%s", key)
}

func marshalAggregateField(ctype metadb.ClusterType, env string) string {
	return fmt.Sprintf("%s:%s", ctype, env)
}

func marshalUnhealthyRWAggregateKey(ctype metadb.ClusterType, aggType types.AggType) string {
	return fmt.Sprintf("unhealthyaggregate:rw:%s:%s", ctype, aggType)
}

func marshalUnhealthyStatusAggregateKey(ctype metadb.ClusterType) string {
	return fmt.Sprintf("unhealthyaggregate:status:%s", ctype)
}

func marshalUnhealthyWarningGeoAggregateKey(ctype metadb.ClusterType) string {
	return fmt.Sprintf("unhealthyaggregate:warningGeo:%s", ctype)
}

func marshalUnhealthyStatusAggregateField(env string, sla bool, status string) string {
	field := fmt.Sprintf("%s:%s", env, status)
	if sla {
		field = "sla_" + field
	}
	return field
}

func marshalUnhealthyWarningGeoAggregateField(env string, sla bool, geo string) string {
	field := fmt.Sprintf("%s:%s", env, geo)
	if sla {
		field = "sla_" + field
	}
	return field
}

func marshalUnhealthyRWAggregateField(env string, sla bool, readable bool, writable bool, userfaultBroken bool) string {
	var field string
	if sla {
		field += slaPrefix
	}
	if readable {
		field += readablePrefix
	}
	if writable {
		field += writablePrefix
	}
	if userfaultBroken {
		field += userfaultBrokenPrefix
	}
	return field + env
}

func marshalServiceField(service string) string {
	return fmt.Sprintf("%s%s", servicePrefix, service)
}

func marshalSystemMetricField(metric string) string {
	return fmt.Sprintf("%s%s", systemMetricsPrefix, metric)
}

func marshalModeField(mode string) string {
	return fmt.Sprintf("%s%s", modePrefix, mode)
}

func marshalSecret(cid string) string {
	return fmt.Sprintf("secret:%s", cid)
}
