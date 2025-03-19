package grpc

import (
	"github.com/golang/protobuf/proto"

	cloudquota "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/quota"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/quota"
)

var resourceToQuotaName = map[quota.ResourceType]string{
	quota.ResourceTypeCPU:      "mdb.cpu.count",
	quota.ResourceTypeGPU:      "mdb.gpu.count",
	quota.ResourceTypeMemory:   "mdb.memory.size",
	quota.ResourceTypeSSDSpace: "mdb.ssd.size",
	quota.ResourceTypeHDDSpace: "mdb.hdd.size",
	quota.ResourceTypeClusters: "mdb.clusters.count",
}

func init() {
	grpcerr.RegisterErrorDetailsConverter(errorDetailsConverter)
}

func errorDetailsConverter(details interface{}) (proto.Message, bool, error) {
	switch typed := details.(type) {
	case *quota.Violations:
		res := cloudquota.QuotaFailure{
			CloudId: typed.CloudExtID,
		}
		for _, v := range typed.Violations {
			res.Violations = append(
				res.Violations,
				&cloudquota.QuotaFailure_Violation{
					Required: v.Required,
					Metric: &cloudquota.QuotaMetric{
						Name:  resourceToQuotaName[v.Resource],
						Limit: v.Limit,
						Usage: v.Usage,
					},
				},
			)
		}

		return &res, true, nil
	}

	return nil, false, nil
}
