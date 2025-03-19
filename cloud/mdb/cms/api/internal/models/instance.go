package models

import "a.yandex-team.ru/cloud/mdb/internal/dbm"

type Instance struct {
	ConductorGroup string
	DBMClusterName string
	FQDN           string
	Volumes        []dbm.Volume
}

func (i Instance) VolumesTotalSpaceLimit() int {
	sum := 0
	if i.Volumes == nil {
		return sum
	}
	for _, v := range i.Volumes {
		sum += v.SpaceLimit
	}
	return sum
}
