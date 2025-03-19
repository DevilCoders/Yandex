package provider

import (
	computeapi "a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	"a.yandex-team.ru/cloud/mdb/internal/compute/vpc"
	"a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/compute"
)

type Compute struct {
	vpc        vpc.Client
	network    network.Client
	auth       auth.Authenticator
	hostGroups computeapi.HostGroupService
	rm         resmanager.Client
	hostType   computeapi.HostTypeService
}

var _ compute.Compute = &Compute{}

func NewCompute(vpc vpc.Client, network network.Client, authenticator auth.Authenticator, hostGroups computeapi.HostGroupService, rm resmanager.Client, hostType computeapi.HostTypeService) *Compute {
	return &Compute{
		vpc:        vpc,
		network:    network,
		auth:       authenticator,
		hostGroups: hostGroups,
		rm:         rm,
		hostType:   hostType,
	}
}
