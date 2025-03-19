package aws

import (
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
)

func CreateStateToNetworkConnectionParams(state *aws.CreateNetworkConnectionOperationState) models.NetworkConnectionParams {
	state.NetworkConnectionParams.ManagedIPv4 = state.Network.IPv4
	state.NetworkConnectionParams.ManagedIPv6 = state.Network.IPv6
	return state.NetworkConnectionParams
}
