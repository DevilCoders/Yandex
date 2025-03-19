package ready

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func WaitForVPCReady(ctx context.Context, f func(ctx context.Context, vpcID string) (aws.Vpc, error), notReadyErr error, vpcID string, l log.Logger) (aws.Vpc, error) {
	checker := NewReadinessChecker(func(ctx context.Context) (CheckingObject, error) {
		return f(ctx, vpcID)
	})
	ctx = ctxlog.WithFields(ctx, log.String("vpcID", vpcID))
	if err := ready.WaitWithTimeout(ctx, 10*time.Second, checker, NewErrorTester(l, notReadyErr), time.Second); err != nil {
		return aws.Vpc{}, err
	} else {
		return checker.GetResult().(aws.Vpc), nil
	}
}
