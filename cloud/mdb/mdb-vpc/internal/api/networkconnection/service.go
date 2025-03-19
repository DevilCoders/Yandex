package networkconnection

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/validation"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
)

type Service struct {
	db   vpcdb.VPCDB
	auth *auth.Auth

	validators map[models.Provider]validation.Validator
}

func (s Service) Update(ctx context.Context, request *network.UpdateNetworkConnectionRequest) (*network.UpdateNetworkConnectionResponse, error) {
	return nil, semerr.NotImplemented("update method is not implemented yet")
}

var _ network.NetworkConnectionServiceServer = &Service{}

func NewService(
	db vpcdb.VPCDB,
	authClient *auth.Auth,
	validators map[models.Provider]validation.Validator,
) network.NetworkConnectionServiceServer {
	return &Service{
		db:         db,
		auth:       authClient,
		validators: validators,
	}
}
