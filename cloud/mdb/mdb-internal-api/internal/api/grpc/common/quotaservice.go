package common

import (
	"context"

	"google.golang.org/protobuf/types/known/emptypb"

	mdbv2 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v2"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/quota"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (q *QuotaService) BatchUpdateMetric(ctx context.Context, request *quota.BatchUpdateQuotaMetricsRequest) (*emptypb.Empty, error) {
	resources, err := metricLimitToResources(request.Metrics)
	if err != nil {
		return nil, semerr.WrapWithInternal(err, "failed to convert metrics")
	}
	err = q.quotas.BatchUpdateMetric(ctx, request.CloudId, resources)
	return &emptypb.Empty{}, err
}

func (q *QuotaService) Get(ctx context.Context, request *quota.GetQuotaRequest) (*quota.Quota, error) {
	cloud, err := q.quotas.Get(ctx, request.CloudId)
	return &quota.Quota{
		CloudId: request.CloudId,
		Metrics: CloudToMetrics(cloud),
	}, err
}

func (q *QuotaService) GetDefault(_ context.Context, _ *quota.GetQuotaDefaultRequest) (*quota.GetQuotaDefaultResponse, error) {
	return nil, semerr.NotImplemented("this is not implemented")
}

type metricName string

const HDDSize metricName = "mdb.hdd.size"
const SSDSize metricName = "mdb.ssd.size"
const GPUCount metricName = "mdb.gpu.count"
const CPUCount metricName = "mdb.cpu.count"
const MemorySize metricName = "mdb.memory.size"
const ClusterCount metricName = "mdb.clusters.count"

func CloudToMetrics(cloud metadb.Cloud) []*quota.QuotaMetric {
	result := make([]*quota.QuotaMetric, 6)
	result[0] = &quota.QuotaMetric{
		Name:  string(HDDSize),
		Limit: cloud.Quota.HDDSpace,
		Usage: float64(cloud.Used.HDDSpace),
	}
	result[1] = &quota.QuotaMetric{
		Name:  string(ClusterCount),
		Limit: cloud.Quota.Clusters,
		Usage: float64(cloud.Used.Clusters),
	}
	result[2] = &quota.QuotaMetric{
		Name:  string(MemorySize),
		Limit: cloud.Quota.Memory,
		Usage: float64(cloud.Used.Memory),
	}
	result[3] = &quota.QuotaMetric{
		Name:  string(CPUCount),
		Limit: int64(cloud.Quota.CPU),
		Usage: float64(cloud.Used.CPU),
	}
	result[4] = &quota.QuotaMetric{
		Name:  string(GPUCount),
		Limit: cloud.Quota.GPU,
		Usage: float64(cloud.Used.GPU),
	}
	result[5] = &quota.QuotaMetric{
		Name:  string(SSDSize),
		Limit: cloud.Quota.SSDSpace,
		Usage: float64(cloud.Used.SSDSpace),
	}
	return result
}

var distrMetricToResource = map[metricName]func(*common.BatchUpdateResources, *quota.MetricLimit){
	HDDSize: func(resources *common.BatchUpdateResources, limit *quota.MetricLimit) {
		resources.HDDSpace.Set(limit.Limit)
	},
	SSDSize: func(resources *common.BatchUpdateResources, limit *quota.MetricLimit) {
		resources.SSDSpace.Set(limit.Limit)
	},
	GPUCount: func(resources *common.BatchUpdateResources, limit *quota.MetricLimit) {
		resources.GPU.Set(limit.Limit)
	},
	CPUCount: func(resources *common.BatchUpdateResources, limit *quota.MetricLimit) {
		resources.CPU.Set(float64(limit.Limit))
	},
	MemorySize: func(resources *common.BatchUpdateResources, limit *quota.MetricLimit) {
		resources.Memory.Set(limit.Limit)
	},
	ClusterCount: func(resources *common.BatchUpdateResources, limit *quota.MetricLimit) {
		resources.Clusters.Set(limit.Limit)
	},
}

var ErrUnknownMetric = xerrors.NewSentinel("unknown metric name")

func metricLimitToResources(limits []*quota.MetricLimit) (common.BatchUpdateResources, error) {
	result := common.BatchUpdateResources{}
	for _, l := range limits {
		setter, ok := distrMetricToResource[metricName(l.Name)]
		if !ok {
			return result, ErrUnknownMetric.Wrap(xerrors.New(l.Name))
		}
		setter(&result, l)
	}
	return result, nil
}

type QuotaService struct {
	quotas common.Quotas
	l      log.Logger
}

func NewQuotaService(
	quotas common.Quotas,
	l log.Logger,
) mdbv2.QuotaServiceServer {
	return &QuotaService{
		quotas: quotas,
		l:      l,
	}
}
