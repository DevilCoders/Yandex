package mysql

import (
	"context"

	myv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mysql/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql"
	"a.yandex-team.ru/library/go/core/log"
)

// PerformanceDiagnosticsService implements DB-specific gRPC methods
type PerformanceDiagnosticsService struct {
	myv1.UnimplementedPerformanceDiagnosticsServiceServer
	l      log.Logger
	mypd   mysql.PerfDiag
	Config api.Config
}

var _ myv1.PerformanceDiagnosticsServiceServer = &PerformanceDiagnosticsService{}

func NewPerformanceDiagnosticsService(mypd mysql.PerfDiag, l log.Logger) *PerformanceDiagnosticsService {
	return &PerformanceDiagnosticsService{mypd: mypd, l: l}
}

func (pds *PerformanceDiagnosticsService) GetSessionsStats(ctx context.Context,
	req *myv1.GetSessionsStatsRequest) (*myv1.GetSessionsStatsResponse, error) {
	args, err := GetSessionsStatsFromGRPC(req)
	var nextPageToken int64
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
	stats, more, err := pds.mypd.GetSessionsStats(ctx, req.GetClusterId(), req.GetPageSize(), args)
	if err != nil {
		return nil, err
	}

	if checkMoreResponse(len(stats), more) {
		nextPageToken = stats[len(stats)-1].NextMessageToken
	} else {
		// If we retrieved zero message, we have to return 'good' token. That token will be whatever
		// we received from caller.
		nextPageToken = offset
	}

	var resps []*myv1.GetSessionsStatsResponse_Stats
	for _, i := range stats {
		resps = append(resps, &myv1.GetSessionsStatsResponse_Stats{
			Time:          grpcapi.OptionalTimeToGRPC(i.Timestamp),
			Dimensions:    i.Dimensions,
			SessionsCount: i.SessionsCount,
		})
	}
	return &myv1.GetSessionsStatsResponse{Stats: resps, NextPageToken: api.PagingTokenToGRPC(nextPageToken)}, err
}

func (pds *PerformanceDiagnosticsService) GetSessionsAtTime(ctx context.Context,
	req *myv1.GetSessionsAtTimeRequest) (*myv1.GetSessionsAtTimeResponse, error) {
	args, err := GetSessionsAtTimeFromGRPC(req)
	var nextPageToken int64
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
	stats, more, err := pds.mypd.GetSessionsAtTime(ctx, req.GetClusterId(), req.GetPageSize(), args)
	if err != nil {
		return nil, err
	}
	if checkMoreResponse(len(stats.Sessions), more) {
		nextPageToken = stats.NextMessageToken
	} else {
		// If we retrieved zero message, we have to return 'good' token. That token will be whatever
		// we received from caller.
		nextPageToken = offset
	}
	resps := &myv1.GetSessionsAtTimeResponse{
		Sessions:            SessionsStateToGRPC(stats.Sessions),
		PreviousCollectTime: grpcapi.TimeToGRPC(stats.PreviousCollectTime),
		NextCollectTime:     grpcapi.TimeToGRPC(stats.NextCollectTime),
		NextPageToken:       api.PagingTokenToGRPC(nextPageToken),
	}
	return resps, err
}

func (pds *PerformanceDiagnosticsService) GetStatementsAtTime(ctx context.Context,
	req *myv1.GetStatementsAtTimeRequest) (*myv1.GetStatementsAtTimeResponse, error) {
	args, err := GetStatementsAtTimeFromGRPC(req)
	var nextPageToken int64
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
	stats, more, err := pds.mypd.GetStatementsAtTime(ctx, req.GetClusterId(), req.GetPageSize(), args)
	if err != nil {
		return nil, err
	}

	if checkMoreResponse(len(stats.Statements), more) {
		nextPageToken = stats.NextMessageToken
	} else {
		// If we retrieved zero message, we have to return 'good' token. That token will be whatever
		// we received from caller.
		nextPageToken = offset
	}
	resps := &myv1.GetStatementsAtTimeResponse{
		Statements:          StatementsAtTimeToGRPC(stats.Statements),
		PreviousCollectTime: grpcapi.TimeToGRPC(stats.PreviousCollectTime),
		NextCollectTime:     grpcapi.TimeToGRPC(stats.NextCollectTime),
		NextPageToken:       api.PagingTokenToGRPC(nextPageToken),
	}
	return resps, err
}

func (pds *PerformanceDiagnosticsService) GetStatementsDiff(ctx context.Context,
	req *myv1.GetStatementsDiffRequest) (*myv1.GetStatementsDiffResponse, error) {
	args, err := GetStatementsDiffFromGRPC(req)
	var nextPageToken int64
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
	stats, more, err := pds.mypd.GetStatementsDiff(ctx, req.GetClusterId(), req.GetPageSize(), args)
	if err != nil {
		return nil, err
	}
	if checkMoreResponse(len(stats.DiffStatements), more) {
		nextPageToken = stats.NextMessageToken
	} else {
		// If we retrieved zero message, we have to return 'good' token. That token will be whatever
		// we received from caller.
		nextPageToken = offset
	}
	resps := &myv1.GetStatementsDiffResponse{
		DiffStatements: StatementsDiffToGRPC(stats.DiffStatements),
		NextPageToken:  api.PagingTokenToGRPC(nextPageToken),
	}
	return resps, err
}

func (pds *PerformanceDiagnosticsService) GetStatementsInterval(ctx context.Context,
	req *myv1.GetStatementsIntervalRequest) (*myv1.GetStatementsIntervalResponse, error) {
	args, err := GetStatementsIntervalFromGRPC(req)
	var nextPageToken int64
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
	stats, more, err := pds.mypd.GetStatementsInterval(ctx, req.GetClusterId(), req.GetPageSize(), args)
	if err != nil {
		return nil, err
	}

	if checkMoreResponse(len(stats.Statements), more) {
		nextPageToken = stats.NextMessageToken
	} else {
		// If we retrieved zero message, we have to return 'good' token. That token will be whatever
		// we received from caller.
		nextPageToken = offset
	}
	resps := &myv1.GetStatementsIntervalResponse{
		Statements:    StatementsAtTimeToGRPC(stats.Statements),
		NextPageToken: api.PagingTokenToGRPC(nextPageToken),
	}
	return resps, err
}

func (pds *PerformanceDiagnosticsService) GetStatementsStats(ctx context.Context,
	req *myv1.GetStatementsStatsRequest) (*myv1.GetStatementsStatsResponse, error) {
	args, err := GetStatementsStatsFromGRPC(req)
	var nextPageToken int64
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
	stats, more, err := pds.mypd.GetStatementsStats(ctx, req.GetClusterId(), req.GetPageSize(), args)
	if err != nil {
		return nil, err
	}

	if checkMoreResponse(len(stats.Statements), more) {
		nextPageToken = stats.NextMessageToken
	} else {
		// If we retrieved zero message, we have to return 'good' token. That token will be whatever
		// we received from caller.
		nextPageToken = offset
	}
	resps := &myv1.GetStatementsStatsResponse{
		Statements:    StatementsStatsGRPC(stats.Statements),
		NextPageToken: api.PagingTokenToGRPC(nextPageToken),
	}
	return resps, err
}

func (pds *PerformanceDiagnosticsService) GetStatementStats(ctx context.Context,
	req *myv1.GetStatementStatsRequest) (*myv1.GetStatementStatsResponse, error) {
	args, err := GetStatementStatsFromGRPC(req)
	var nextPageToken int64
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
	stats, more, err := pds.mypd.GetStatementStats(ctx, req.GetClusterId(), req.GetPageSize(), args)
	if err != nil {
		return nil, err
	}

	if checkMoreResponse(len(stats.Statements), more) {
		nextPageToken = stats.NextMessageToken
	} else {
		// If we retrieved zero message, we have to return 'good' token. That token will be whatever
		// we received from caller.
		nextPageToken = offset
	}
	resps := &myv1.GetStatementStatsResponse{
		Query:         stats.Query,
		Statements:    StatementsStatsGRPC(stats.Statements),
		NextPageToken: api.PagingTokenToGRPC(nextPageToken),
	}
	return resps, err
}

func checkMoreResponse(l int, more bool) bool {
	if l > 0 && more {
		return true
	}
	return false
}
