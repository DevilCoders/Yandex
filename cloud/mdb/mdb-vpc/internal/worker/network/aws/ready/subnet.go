package ready

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func WaitForSubnetReady(ctx context.Context, f func(ctx context.Context, subnetID string) (aws.Subnet, error), notReadyErr error, subnetID string, l log.Logger) (aws.Subnet, error) {
	checker := NewReadinessChecker(func(ctx context.Context) (CheckingObject, error) {
		return f(ctx, subnetID)
	})
	ctx = ctxlog.WithFields(ctx, log.String("subnetID", subnetID))
	if err := ready.WaitWithTimeout(ctx, 10*time.Second, checker, NewErrorTester(l, notReadyErr), time.Second); err != nil {
		return aws.Subnet{}, err
	} else {
		return checker.GetResult().(aws.Subnet), nil
	}
}
