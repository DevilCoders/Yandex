package validation

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
)

type Validator interface {
	ValidateImportVPCData(ctx context.Context, params interface{}) (ValidateImportVPCResult, error)
	ValidateCreateNetworkConnectionData(ctx context.Context, net models.Network, params interface{}) (ValidateNetworkConnectionCreationResult, error)
}

type ValidateImportVPCResult struct {
	IPv4CIDRBlock string
}

type ValidateNetworkConnectionCreationResult struct {
	PeeringID string
}
