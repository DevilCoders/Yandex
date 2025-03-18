package consoleapi

import (
	"context"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/types/known/emptypb"

	cdnpb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/cdn/v1/console"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type CacheServiceHandler struct {
	Logger log.Logger
}

func (c *CacheServiceHandler) Purge(ctx context.Context, request *cdnpb.CacheToolsRequest) (*emptypb.Empty, error) {
	ctxlog.Info(ctx, c.Logger, "cache purge", log.Sprintf("request", "%s", request))
	return nil, status.Error(codes.Unimplemented, "Not implemented")
}

func (c *CacheServiceHandler) Prefetch(ctx context.Context, request *cdnpb.CacheToolsRequest) (*emptypb.Empty, error) {
	ctxlog.Info(ctx, c.Logger, "cache prefetch", log.Sprintf("request", "%s", request))
	return nil, status.Error(codes.Unimplemented, "Not implemented")
}
