package consoleapi

import (
	"context"
	"math/rand"
	"strconv"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/proto"

	"a.yandex-team.ru/cdn/cloud_api/pkg/handler/grpcutil"
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/auth"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/resourceservice"
	cdnpb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/cdn/v1/console"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

// TODO: at the moment it is a stub

type ResourceMetricsServiceHandler struct {
	Logger          log.Logger
	AuthClient      grpcutil.AuthClient
	ResourceService resourceservice.ResourceService
}

func (h *ResourceMetricsServiceHandler) Get(
	ctx context.Context,
	request *cdnpb.GetResourceMetricsRequest,
) (*cdnpb.ResourceMetricsResponse, error) {
	ctxlog.Info(
		ctx, h.Logger,
		"get resource metrics request",
		log.String("resource_id", request.GetResourceId()),
		log.String("parameters", request.GetParameters().String()),
	)

	params := &model.GetResourceParams{
		ResourceID: request.ResourceId,
	}

	resource, errorResult := h.ResourceService.GetResource(ctx, params)
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("get resource", errorResult)
	}

	err := h.AuthClient.Authorize(ctx, auth.GetResourcePermission, entityResource(resource.FolderID, resource.ID))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	parameters := request.GetParameters()
	if parameters == nil {
		return nil, status.Error(codes.InvalidArgument, "parameters are not specified")
	}

	period := parameters.GetPeriod()
	if period == nil {
		return nil, status.Error(codes.InvalidArgument, "period is not specified")
	}

	aggregates, err := resourceMetricsGetAggregates(period)
	if err != nil {
		return nil, err
	}

	resourceMetrics := &cdnpb.ResourceMetricsResponse{
		FolderId:   strconv.Itoa(rand.Int()),
		ResourceId: request.GetResourceId(),
		Aggregates: aggregates,
	}

	return resourceMetrics, nil
}

func (h *ResourceMetricsServiceHandler) List(
	ctx context.Context,
	request *cdnpb.ListResourcesMetricsRequest,
) (*cdnpb.ListResourcesMetricsResponse, error) {
	ctxlog.Info(
		ctx, h.Logger,
		"list resources metrics request",
		log.String("folder_id", request.GetFolderId()),
		log.String("parameters", request.GetParameters().String()),
	)

	err := h.AuthClient.Authorize(ctx, auth.ListResourcesPermission, folder(request.FolderId))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	parameters := request.GetParameters()
	if parameters == nil {
		return nil, status.Error(codes.InvalidArgument, "parameters are not specified")
	}

	period := parameters.GetPeriod()
	if period == nil {
		return nil, status.Error(codes.InvalidArgument, "period is not specified")
	}

	aggregates, err := resourceMetricsGetAggregates(period)
	if err != nil {
		return nil, err
	}

	resourcesMetrics := &cdnpb.ListResourcesMetricsResponse{
		Metrics: []*cdnpb.ResourceMetricsResponse{
			{
				FolderId:   request.GetFolderId(),
				ResourceId: strconv.Itoa(rand.Int()),
				Aggregates: aggregates,
			},
		},
	}

	return resourcesMetrics, nil
}

func (h *ResourceMetricsServiceHandler) GetFolderStats(
	ctx context.Context,
	request *cdnpb.GetFolderStatsRequest,
) (*cdnpb.GetFolderStatsResponse, error) {
	ctxlog.Info(ctx, h.Logger, "get folder stats request", log.String("folder_id", request.GetFolderId()))

	err := h.AuthClient.Authorize(ctx, auth.ListResourcesPermission, folder(request.GetFolderId()))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	aggregate := proto.Clone(resourceMetricsAggregateMock).(*cdnpb.ResourceMetricsAggregate)

	params := &model.CountActiveResourcesParams{
		FolderID: request.GetFolderId(),
	}
	count, errorResult := h.ResourceService.CountActiveResourcesByFolderID(ctx, params)
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("get folder stats", errorResult)
	}

	stats := &cdnpb.GetFolderStatsResponse{
		FolderId:        request.GetFolderId(),
		ResourcesCount:  count,
		MonthAggregates: aggregate,
	}

	return stats, nil
}

var resourceMetricsAggregateMock = &cdnpb.ResourceMetricsAggregate{
	UpstreamBytes: &cdnpb.ResourceMetricsAggregate_ResourceMetricCounter{Value: 0},
	SendBytes:     &cdnpb.ResourceMetricsAggregate_ResourceMetricCounter{Value: 0},
	TotalBytes:    &cdnpb.ResourceMetricsAggregate_ResourceMetricCounter{Value: 0},
	Requests:      &cdnpb.ResourceMetricsAggregate_ResourceMetricCounter{Value: 0},
	Responses_2Xx: &cdnpb.ResourceMetricsAggregate_ResourceMetricCounter{Value: 0},
	Responses_3Xx: &cdnpb.ResourceMetricsAggregate_ResourceMetricCounter{Value: 0},
	Responses_4Xx: &cdnpb.ResourceMetricsAggregate_ResourceMetricCounter{Value: 0},
	Responses_5Xx: &cdnpb.ResourceMetricsAggregate_ResourceMetricCounter{Value: 0},
	ResponsesHit:  &cdnpb.ResourceMetricsAggregate_ResourceMetricCounter{Value: 0},
	ResponsesMiss: &cdnpb.ResourceMetricsAggregate_ResourceMetricCounter{Value: 0},
	CacheHitRatio: &cdnpb.ResourceMetricsAggregate_ResourceMetricGauge{Value: 0},
	ShieldBytes:   &cdnpb.ResourceMetricsAggregate_ResourceMetricCounter{Value: 0},
}

func resourceMetricsGetAggregates(period *cdnpb.ResourceMetricsParameter_RequestPeriod) (*cdnpb.ResourceMetricsResponse_Aggregates, error) {
	aggregate := proto.Clone(resourceMetricsAggregateMock).(*cdnpb.ResourceMetricsAggregate)
	aggregates := &cdnpb.ResourceMetricsResponse_Aggregates{}

	switch period := period.GetPeriodVariant().(type) {
	case *cdnpb.ResourceMetricsParameter_RequestPeriod_CustomPeriod:
		aggregates.Interval = aggregate
	case *cdnpb.ResourceMetricsParameter_RequestPeriod_DefaultPeriod:
		switch period.DefaultPeriod {
		case cdnpb.ResourceMetricsParameter_PREDEFINED_PERIODS_UNSPECIFIED:
			return nil, status.Error(codes.InvalidArgument, "requested predefined periods should be specified")
		case cdnpb.ResourceMetricsParameter_PREDEFINED_PERIODS_DAY:
			aggregates.Day = aggregate
		case cdnpb.ResourceMetricsParameter_PREDEFINED_PERIODS_WEEK:
			aggregates.Week = aggregate
		case cdnpb.ResourceMetricsParameter_PREDEFINED_PERIODS_MONTH:
			aggregates.Month = aggregate
		case cdnpb.ResourceMetricsParameter_PREDEFINED_PERIODS_ALL:
			return nil, status.Error(codes.Unimplemented, "all aggregate is not supported")
		default:
			return nil, status.Errorf(codes.InvalidArgument, "unknown period: %s", period.DefaultPeriod)
		}
	default:
		return nil, status.Errorf(codes.InvalidArgument, "unknown request period type: %T", period)
	}

	return aggregates, nil
}
