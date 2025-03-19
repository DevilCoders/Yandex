package network

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	awsmodels "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Service) Import(ctx context.Context, req *network.ImportNetworkRequest) (*network.ImportNetworkResponse, error) {
	author, err := s.auth.Authorize(ctx, auth.NetworkCreatePermission, req.ProjectId)
	if err != nil {
		return nil, err
	}

	var provider models.Provider
	var region string
	var er models.ExternalResources
	switch requestParams := req.Params.(type) {
	case *network.ImportNetworkRequest_Aws:
		provider = models.ProviderAWS
		region = requestParams.Aws.RegionId
		er = &awsmodels.NetworkExternalResources{
			VpcID:      requestParams.Aws.VpcId,
			AccountID:  optional.NewString(requestParams.Aws.AccountId),
			IamRoleArn: optional.NewString(requestParams.Aws.IamRoleArn),
		}
	default:
		return nil, semerr.InvalidInput("unsupported cloud_type")
	}

	validator, ok := s.validators[provider]
	if !ok {
		return nil, xerrors.Errorf("can not find validator for provider %q", provider)
	}

	valRes, err := validator.ValidateImportVPCData(ctx, req.Params)
	if err != nil {
		return nil, semerr.WrapWithInvalidInputf(err, "validation error: %s", err)
	}

	ctx, err = s.db.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return nil, err
	}

	networkID, err := s.db.CreateNetwork(
		ctx,
		req.ProjectId,
		provider,
		region,
		req.Name,
		req.Description,
		valRes.IPv4CIDRBlock,
		er,
	)
	if err != nil {
		return nil, err
	}

	var params models.OperationParams
	switch provider {
	case models.ProviderAWS:
		requestParams := req.Params.(*network.ImportNetworkRequest_Aws)
		params = &awsmodels.ImportVPCOperationParams{
			NetworkID:  networkID,
			VpcID:      requestParams.Aws.VpcId,
			IamRoleArn: requestParams.Aws.IamRoleArn,
			AccountID:  requestParams.Aws.AccountId,
		}

	default:
		return nil, semerr.InvalidInput("unsupported cloud_type")
	}

	opID, err := s.db.InsertOperation(
		ctx,
		req.ProjectId,
		"import operation",
		author,
		params,
		models.OperationActionImportVPC,
		provider,
		region,
	)
	if err != nil {
		return nil, err
	}

	err = s.db.Commit(ctx)
	if err != nil {
		return nil, err
	}

	return &network.ImportNetworkResponse{NetworkId: networkID, OperationId: opID}, nil
}
