package mongodb

import (
	"context"

	mongov1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mongodb/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb"
	"a.yandex-team.ru/library/go/core/log"
)

// PerformanceDiagnosticsService implements DB-specific gRPC methods
type PerformanceDiagnosticsService struct {
	mongov1.UnimplementedPerformanceDiagnosticsServiceServer
	l      log.Logger
	mg     mongodb.PerfDiag
	Config api.Config
}

var _ mongov1.PerformanceDiagnosticsServiceServer = &PerformanceDiagnosticsService{}

func NewPerformanceDiagnosticsService(mg mongodb.PerfDiag, l log.Logger) *PerformanceDiagnosticsService {
	return &PerformanceDiagnosticsService{mg: mg, l: l}
}

func (pds *PerformanceDiagnosticsService) GetProfilerStats(ctx context.Context,
	req *mongov1.GetProfilerStatsRequest) (*mongov1.GetProfilerStatsResponse, error) {
	var nextPageToken int64
	args, err := GetProfilerStatsFromGRPC(req)
	if err != nil {
		return nil, err
	}

	offset, ok, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}
	if ok {
		args.Offset.Set(offset)
	}

	data, more, err := pds.mg.GetProfilerStats(ctx, req.GetClusterId(), args)
	if err != nil {
		return nil, err
	}

	if more {
		nextPageToken = offset + int64(len(data))
	} else {
		// If we retrieved zero message, we have to return 'good' token. That token will be whatever
		// we received from caller.
		nextPageToken = offset
	}

	var resps []*mongov1.ProfilerStats
	for _, i := range data {
		resps = append(resps, &mongov1.ProfilerStats{
			Time:       grpcapi.TimeToGRPC(i.Time),
			Dimensions: i.Dimensions,
			Value:      i.Value,
		})
	}
	return &mongov1.GetProfilerStatsResponse{Data: resps, NextPageToken: api.PagingTokenToGRPC(nextPageToken)}, err
}

func (pds *PerformanceDiagnosticsService) GetProfilerRecsAtTime(ctx context.Context,
	req *mongov1.GetProfilerRecsAtTimeRequest) (*mongov1.GetProfilerRecsAtTimeResponse, error) {
	var nextPageToken int64
	args, err := GetProfilerRecsAtTimeFromGRPC(req)
	if err != nil {
		return nil, err
	}

	offset, ok, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}
	if ok {
		args.Offset.Set(offset)
	}
	data, more, err := pds.mg.GetProfilerRecsAtTime(ctx, req.GetClusterId(), args)

	if err != nil {
		return nil, err
	}

	if more {
		nextPageToken = offset + int64(len(data))
	} else {
		// If we retrieved zero message, we have to return 'good' token. That token will be whatever
		// we received from caller.
		nextPageToken = offset
	}

	var resps []*mongov1.ProfilerRecord
	for _, i := range data {
		resps = append(resps, &mongov1.ProfilerRecord{
			Time:              grpcapi.TimeToGRPC(i.Time),
			Raw:               i.RawRequest,
			RequestForm:       i.RequestForm,
			Hostname:          i.Hostname,
			User:              i.User,
			Ns:                i.Namespace,
			Operation:         i.Operation,
			Duration:          i.Duration,
			PlanSummary:       i.PlanSummary,
			ResponseLength:    i.ResponseLength,
			KeysExamined:      i.KeysExamined,
			DocumentsExamined: i.DocumentsExamined,
			DocumentsReturned: i.DocumentsReturned,
		})
	}
	return &mongov1.GetProfilerRecsAtTimeResponse{Data: resps, NextPageToken: api.PagingTokenToGRPC(nextPageToken)}, err
}

func (pds *PerformanceDiagnosticsService) GetProfilerTopFormsByStat(ctx context.Context,
	req *mongov1.GetProfilerTopFormsByStatRequest) (*mongov1.GetProfilerTopFormsByStatResponse, error) {
	var nextPageToken int64
	args, err := GetProfilerTopFormsByStatFromGRPC(req)
	if err != nil {
		return nil, err
	}

	offset, ok, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}
	if ok {
		args.Offset.Set(offset)
	}

	data, more, err := pds.mg.GetProfilerTopFormsByStat(ctx, req.GetClusterId(), args)
	if err != nil {
		return nil, err
	}

	if more {
		nextPageToken = offset + int64(len(data))
	} else {
		// If we retrieved zero message, we have to return 'good' token. That token will be whatever
		// we received from caller.
		nextPageToken = offset
	}

	var resps []*mongov1.TopForms
	for _, i := range data {
		resps = append(resps, &mongov1.TopForms{
			RequestForm:                          i.RequestForm,
			PlanSummary:                          i.PlanSummary,
			QueriesCount:                         i.QueriesCount,
			TotalQueriesDuration:                 i.TotalQueriesDuration,
			AvgQueryDuration:                     i.AVGQueryDuration,
			TotalResponseLength:                  i.TotalResponseLength,
			AvgResponseLength:                    i.AVGResponseLength,
			TotalKeysExamined:                    i.TotalKeysExamined,
			TotalDocumentsExamined:               i.TotalDocumentsExamined,
			TotalDocumentsReturned:               i.TotalDocumentsReturned,
			KeysExaminedPerDocumentReturned:      i.KeysExaminedPerDocumentReturned,
			DocumentsExaminedPerDocumentReturned: i.DocumentsExaminedPerDocumentReturned,
		})
	}
	return &mongov1.GetProfilerTopFormsByStatResponse{Data: resps, NextPageToken: api.PagingTokenToGRPC(nextPageToken)}, err
}

func (pds *PerformanceDiagnosticsService) GetPossibleIndexes(ctx context.Context,
	req *mongov1.GetPossibleIndexesRequest) (*mongov1.GetPossibleIndexesResponse, error) {
	var nextPageToken int64
	args, err := GetPossibleIndexesFromGRPC(req)
	if err != nil {
		return nil, err
	}

	offset, ok, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, err
	}
	if ok {
		args.Offset.Set(offset)
	}

	data, more, err := pds.mg.GetPossibleIndexes(ctx, req.GetClusterId(), args)
	if err != nil {
		return nil, err
	}

	if more {
		nextPageToken = offset + int64(len(data))
	} else {
		// If we retrieved zero message, we have to return 'good' token. That token will be whatever
		// we received from caller.
		nextPageToken = offset
	}

	var resps []*mongov1.PossibleIndexes
	for _, i := range data {
		resps = append(resps, &mongov1.PossibleIndexes{
			Database:     i.Database,
			Collection:   i.Collection,
			Index:        i.Index,
			RequestCount: i.RequestCount,
		})
	}
	return &mongov1.GetPossibleIndexesResponse{Data: resps, NextPageToken: api.PagingTokenToGRPC(nextPageToken)}, err
}

func (pds *PerformanceDiagnosticsService) GetValidOperations(ctx context.Context,
	req *mongov1.GetValidOperationsRequest) (*mongov1.GetValidOperationsResponse, error) {

	data, err := pds.mg.GetValidOperations(ctx, req.GetClusterId())
	if err != nil {
		return nil, err
	}

	var resps []string
	for _, i := range data {
		resps = append(resps, string(i))
	}
	return &mongov1.GetValidOperationsResponse{Op: resps}, err
}
