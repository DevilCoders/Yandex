package networkconnection

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s Service) Create(ctx context.Context, req *network.CreateNetworkConnectionRequest) (*network.CreateNetworkConnectionResponse, error) {
	ctx, err := s.db.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return nil, err
	}

	net, err := s.db.NetworkByID(ctx, req.NetworkId)
	if err != nil {
		return nil, err
	}

	author, err := s.auth.Authorize(ctx, auth.NetworkConnectionCreatePermission, net.ProjectID)
	if err != nil {
		return nil, err
	}

	var provider models.Provider
	var params models.NetworkConnectionParams
	switch requestParams := req.Params.(type) {
	case *network.CreateNetworkConnectionRequest_Aws:
		provider = models.ProviderAWS
		switch ncType := requestParams.Aws.Type.(type) {
		case *network.CreateAWSNetworkConnectionRequest_Peering:
			validator, ok := s.validators[provider]
			if !ok {
				return nil, xerrors.Errorf("can not find validator for provider %q", provider)
			}
			validRes, err := validator.ValidateCreateNetworkConnectionData(ctx, net, req.Params)
			if err != nil {
				return nil, semerr.WrapWithInvalidInputf(err, "validation error: %s", err)
			}

			params = aws.NewNetworkConnectionPeeringParams(
				ncType.Peering.AccountId,
				ncType.Peering.VpcId,
				ncType.Peering.Ipv4CidrBlock,
				ncType.Peering.Ipv6CidrBlock,
				ncType.Peering.RegionId,
				validRes.PeeringID,
			)
		default:
			return nil, semerr.InvalidInput("unsupported Network Connection type")
		}
	default:
		return nil, semerr.InvalidInput("unsupported provider")
	}

	if net.Provider != provider {
		return nil, semerr.InvalidInputf("Network provider is %q while Network Connection provider in %q", net.Provider, provider)
	}

	ncID, err := s.db.CreateNetworkConnection(ctx, req.NetworkId, net.ProjectID, provider, net.Region, req.Description, params)
	if err != nil {
		return nil, err
	}

	opID, err := s.db.InsertOperation(
		ctx,
		net.ProjectID,
		"create network connection operation",
		author,
		&models.CreateNetworkConnectionOperationParams{NetworkConnectionID: ncID},
		models.OperationActionCreateNetworkConnection,
		net.Provider,
		net.Region,
	)
	if err != nil {
		return nil, err
	}

	err = s.db.Commit(ctx)
	if err != nil {
		return nil, err
	}

	return &network.CreateNetworkConnectionResponse{NetworkConnectionId: ncID, OperationId: opID}, nil
}
