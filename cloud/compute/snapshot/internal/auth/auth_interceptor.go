package auth

import (
	"context"

	"go.uber.org/zap"
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

const (
	iamPermission = "compute.snapshotService.full"
)

func NewServerInterceptor(accessClient AccessServiceClient) grpc.UnaryServerInterceptor {
	s := serverInterceptor{
		access: accessClient,
	}
	return s.UnaryServerInterceptor
}

type serverInterceptor struct {
	access AccessServiceClient
}

func (s serverInterceptor) UnaryServerInterceptor(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (resp interface{}, err error) {
	logger := ctxlog.G(ctx)
	t := misc.AuthorizeTimer.Start()
	user, err := s.access.Authorize(ctx, iamPermission)
	t.ObserveDuration()
	ctxlog.DebugError(logger, err, "Authorize", zap.String("user", user))
	if err != nil {
		misc.AuthorizationFailed.Inc()
		return nil, err
	}
	misc.AuthorizationOk.Inc()

	resp, err = handler(ctx, req)
	return resp, err
}
