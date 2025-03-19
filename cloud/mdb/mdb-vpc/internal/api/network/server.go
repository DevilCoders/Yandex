package network

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/config"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/validation"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
)

type Service struct {
	db     vpcdb.VPCDB
	awsCfs config.Aws
	auth   *auth.Auth

	validators map[models.Provider]validation.Validator
}

func (s Service) Update(ctx context.Context, request *network.UpdateNetworkRequest) (*network.UpdateNetworkResponse, error) {
	return nil, semerr.NotImplemented("update method is not implemented yet")
}

var _ network.NetworkServiceServer = &Service{}

func NewService(
	db vpcdb.VPCDB,
	awsCfs config.Aws,
	authClient *auth.Auth,
	validators map[models.Provider]validation.Validator,
) network.NetworkServiceServer {
	return &Service{
		db:         db,
		awsCfs:     awsCfs,
		auth:       authClient,
		validators: validators,
	}
}
