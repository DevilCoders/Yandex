package ready

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func WaitForTgwAttachmentReady(
	ctx context.Context,
	f func(ctx context.Context, tgwAttachmentID string) error,
	notReadyErr error,
	tgwAttachmentID string,
	l log.Logger,
) error {
	checker := NewReadinessChecker(func(ctx context.Context) (CheckingObject, error) {
		return nil, f(ctx, tgwAttachmentID)
	})
	ctx = ctxlog.WithFields(ctx, log.String("tgwAttachmentID", tgwAttachmentID))
	return ready.WaitWithTimeout(ctx, 10*time.Second, checker, NewErrorTester(l, notReadyErr), time.Second)
}

func WaitForTgwRouteTableAssociationReady(
	ctx context.Context,
	f func(ctx context.Context, routeTableID string, vpcID string) error,
	notReadyErr error,
	routeTableID string,
	vpcID string,
	l log.Logger,
) error {
	checker := NewReadinessChecker(func(ctx context.Context) (CheckingObject, error) {
		return nil, f(ctx, routeTableID, vpcID)
	})
	ctx = ctxlog.WithFields(ctx, log.String("routeTableID", routeTableID), log.String("vpcID", vpcID))
	return ready.WaitWithTimeout(ctx, 10*time.Second, checker, NewErrorTester(l, notReadyErr), time.Second)
}
