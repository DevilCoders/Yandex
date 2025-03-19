package redis

import (
	"encoding/json"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func unmarshalTopologyField(prefix topologyPrefix, field string) (string, bool) {
	if !strings.HasPrefix(field, string(prefix)) {
		return "", false
	}
	return strings.TrimPrefix(field, string(prefix)), true
}

func unmarshalAggregateField(field string) (string, string, error) {
	fsl := strings.Split(field, ":")
	if len(fsl) != 2 {
		err := xerrors.Errorf("invalid aggregate field %s", field)
		return "", "", err
	}
	return fsl[0], fsl[1], nil
}

func unmarshalUnhealthyStatusAggregateField(field string) (env string, sla bool, status string, err error) {
	sla = strings.HasPrefix(field, slaPrefix)
	if sla {
		field = strings.TrimPrefix(field, slaPrefix)
	}
	esl := strings.Split(field, ":")
	if len(esl) != 2 {
		err := xerrors.Errorf("invalid aggregate field %s", field)
		return "", false, "", err
	}
	return esl[0], sla, esl[1], nil
}

func unmarshalUnhealthyWarningGeoAggregateField(field string) (env string, sla bool, geo string, err error) {
	sla = strings.HasPrefix(field, slaPrefix)
	if sla {
		field = strings.TrimPrefix(field, slaPrefix)
	}
	esl := strings.Split(field, ":")
	if len(esl) != 2 {
		err := xerrors.Errorf("invalid aggregate field %s", field)
		return "", false, "", err
	}
	return esl[0], sla, esl[1], nil
}

func unmarshalUnhealthyRWAggregateField(field string) (env string, sla bool, readable bool, writable bool, userfaultBroken bool) {
	sla = strings.HasPrefix(field, slaPrefix)
	if sla {
		field = strings.TrimPrefix(field, slaPrefix)
	}
	readable = strings.HasPrefix(field, readablePrefix)
	if readable {
		field = strings.TrimPrefix(field, readablePrefix)
	}
	writable = strings.HasPrefix(field, writablePrefix)
	if writable {
		field = strings.TrimPrefix(field, writablePrefix)
	}
	userfaultBroken = strings.HasPrefix(field, userfaultBrokenPrefix)
	if userfaultBroken {
		field = strings.TrimPrefix(field, userfaultBrokenPrefix)
	}
	return field, sla, readable, writable, userfaultBroken
}

func unmarshalFqdn(fqdnHealthField string) (string, string, error) {
	ind := strings.Index(fqdnHealthField, ":")
	if ind < 0 {
		return "", "", xerrors.Errorf("could not find delemiter ':'")
	}
	return fqdnHealthField[:ind], fqdnHealthField[ind+1:], nil
}

func unmarshalHostService(fqdn, service, serviceHealthJSON string) (types.ServiceHealth, error) {
	var ssh storeServiceHealth
	err := json.Unmarshal([]byte(serviceHealthJSON), &ssh)
	if err != nil {
		err := xerrors.Errorf("failed to unmarshal health service %s for fqdn %s: %s", service, fqdn, err)
		return types.NewUnknownHealthForService(service), err
	}
	sh := types.NewServiceHealth(service, time.Unix(ssh.Timestamp, 0), ssh.Status, ssh.Role, ssh.ReplicaType, ssh.ReplicaUpstream, ssh.ReplicaLag, ssh.Metrics)
	return sh, nil
}

func unmarshalFQDNList(fqdnList string) []string {
	return strings.Split(fqdnList, " ")
}
