package console

import (
	"context"

	consolev1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v1/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
)

type NetworkService struct {
	consolev1.NetworkServiceServer

	Console console.Console
}

var _ consolev1.NetworkServiceServer = &NetworkService{}

func (ns *NetworkService) List(ctx context.Context, req *consolev1.ListNetworksRequest) (*consolev1.ListNetworksResponse, error) {
	networks, err := ns.Console.GetNetworksByCloudExtID(ctx, req.CloudId)
	if err != nil {
		return nil, err
	}

	resp := &consolev1.ListNetworksResponse{}

	resp.NetworkIds = append(resp.NetworkIds, networks...)

	return resp, nil
}
