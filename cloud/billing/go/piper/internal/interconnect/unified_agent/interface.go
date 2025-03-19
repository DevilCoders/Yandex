package unifiedagent

import (
	"context"
)

//go:generate mockery --name UAClient

type UAClient interface {
	PushMetrics(ctx context.Context, metrics ...SolomonMetric) error
	HealthCheck(context.Context) error
}
