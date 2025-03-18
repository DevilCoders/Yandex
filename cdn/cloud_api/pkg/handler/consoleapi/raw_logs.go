package consoleapi

import (
	"context"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/types/known/emptypb"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1"
	cdnpb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/cdn/v1/console"
)

type RawLogsServiceHandler struct {
}

func (r *RawLogsServiceHandler) Activate(ctx context.Context, request *cdnpb.ActivateRawLogsRequest) (*cdnpb.ActivateRawLogsResponse, error) {
	return nil, status.Error(codes.Unimplemented, "Not implemented")
}

func (r *RawLogsServiceHandler) Deactivate(ctx context.Context, request *cdnpb.DeactivateRawLogsRequest) (*emptypb.Empty, error) {
	return nil, status.Error(codes.Unimplemented, "Not implemented")
}

func (r *RawLogsServiceHandler) Get(ctx context.Context, request *cdnpb.GetRawLogsRequest) (*cdnpb.GetRawLogsResponse, error) {
	return nil, status.Error(codes.Unimplemented, "Not implemented")
}

func (r *RawLogsServiceHandler) Update(ctx context.Context, request *cdnpb.UpdateRawLogsRequest) (*cdnpb.UpdateRawLogsResponse, error) {
	return nil, status.Error(codes.Unimplemented, "Not implemented")
}

func (r *RawLogsServiceHandler) SimulateBillingMetrics(ctx context.Context, request *cdnpb.ActivateRawLogsRequest) (*billing.ConsoleLightMetricsListResponse, error) {
	return nil, status.Error(codes.Unimplemented, "Not implemented")
}
