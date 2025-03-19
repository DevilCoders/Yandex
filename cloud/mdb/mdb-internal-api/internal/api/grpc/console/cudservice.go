package console

import (
	"context"

	consolev1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v1/console"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
)

// CUDService implements gRPC methods for CUD console helpers
// CUD is committed use discount
type CUDService struct {
	consolev1.UnimplementedCUDServiceServer

	Console console.Console
}

var _ consolev1.CUDServiceServer = &CUDService{}

// GetPlatforms returns available MDB platforms
func (cs *CUDService) GetPlatforms(ctx context.Context, req *consolev1.GetPlatformsRequest) (*consolev1.GetPlatformsResponse, error) {
	platforms, err := cs.Console.GetPlatforms(ctx)
	if err != nil {
		return nil, err
	}

	var respPlatforms []*consolev1.GetPlatformsResponse_PlatformInfo
	for _, pl := range platforms {
		respPlatforms = append(
			respPlatforms,
			&consolev1.GetPlatformsResponse_PlatformInfo{
				PlatformId:  string(pl.ID),
				Description: pl.Description,
				Generation:  pl.Generation,
			})
	}

	return &consolev1.GetPlatformsResponse{Platforms: respPlatforms}, nil
}

// GetBillingMetrics returns metrics for CUD console handlers, in format suitable for sending to billing
//
// This handler does not check auth but it seems to be harmless
func (cs *CUDService) GetBillingMetrics(ctx context.Context, req *consolev1.GetBillingMetricsRequest) (*consolev1.GetBillingMetricsResponse, error) {
	cType, err := clusterTypeFromGRPCToModel(req.ClusterType)
	if err != nil {
		return &consolev1.GetBillingMetricsResponse{}, err
	}

	// Billing expects an array for roles but we always return just one role
	roles := []string{cType.Roles().Main.Stringified()}

	response := &consolev1.GetBillingMetricsResponse{
		Schema: consolemodels.BillingSchemaNameYandexCloud,
		Tags: &consolev1.GetBillingMetricsResponse_BillingTags{
			ClusterType:  cType.Stringified(),
			Roles:        roles,
			PlatformId:   req.PlatformId,
			Cores:        req.Cores,
			CoreFraction: 100, // We don't allow CUDs for burstable
			Memory:       req.Memory,
			Online:       1,
			Edition:      req.Edition,
		},
	}

	return response, nil
}

func clusterTypeFromGRPCToModel(grpcType consolev1.GetBillingMetricsRequest_Type) (clusters.Type, error) {
	switch grpcType {
	case consolev1.GetBillingMetricsRequest_CLICKHOUSE:
		return clusters.TypeClickHouse, nil
	case consolev1.GetBillingMetricsRequest_POSTGRESQL:
		return clusters.TypePostgreSQL, nil
	case consolev1.GetBillingMetricsRequest_MONGODB:
		return clusters.TypeMongoDB, nil
	case consolev1.GetBillingMetricsRequest_REDIS:
		return clusters.TypeRedis, nil
	case consolev1.GetBillingMetricsRequest_MYSQL:
		return clusters.TypeMySQL, nil
	case consolev1.GetBillingMetricsRequest_KAFKA:
		return clusters.TypeKafka, nil
	case consolev1.GetBillingMetricsRequest_ELASTICSEARCH:
		return clusters.TypeElasticSearch, nil
	default:
		return clusters.TypeUnknown, semerr.NotImplementedf("unknown cluster type: %s", grpcType)
	}
}
