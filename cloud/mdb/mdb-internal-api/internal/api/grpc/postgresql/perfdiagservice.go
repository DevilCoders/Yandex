package postgresql

import (
	"context"

	pgv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/postgresql/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql"
	"a.yandex-team.ru/library/go/core/log"
)

// PerformanceDiagnosticsService implements DB-specific gRPC methods
type PerformanceDiagnosticsService struct {
	pgv1.UnimplementedPerformanceDiagnosticsServiceServer
	l      log.Logger
	pg     postgresql.PerfDiag
	Config api.Config
}

var _ pgv1.PerformanceDiagnosticsServiceServer = &PerformanceDiagnosticsService{}

func NewPerformanceDiagnosticsService(pg postgresql.PerfDiag, l log.Logger) *PerformanceDiagnosticsService {
	return &PerformanceDiagnosticsService{pg: pg, l: l}
}

func (pds *PerformanceDiagnosticsService) GetSessionsStats(ctx context.Context,
	req *pgv1.GetSessionsStatsRequest) (*pgv1.GetSessionsStatsResponse, error) {
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
	stats, more, err := pds.pg.GetSessionsStats(ctx, req.GetClusterId(), req.GetPageSize(), args)
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

	var resps []*pgv1.GetSessionsStatsResponse_Stats
	for _, i := range stats {
		resps = append(resps, &pgv1.GetSessionsStatsResponse_Stats{
			Time:          grpcapi.OptionalTimeToGRPC(i.Timestamp),
			Dimensions:    i.Dimensions,
			SessionsCount: i.SessionsCount,
		})
	}
	return &pgv1.GetSessionsStatsResponse{Stats: resps, NextPageToken: api.PagingTokenToGRPC(nextPageToken)}, err
}

func (pds *PerformanceDiagnosticsService) GetSessionsAtTime(ctx context.Context,
	req *pgv1.GetSessionsAtTimeRequest) (*pgv1.GetSessionsAtTimeResponse, error) {
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
	stats, more, err := pds.pg.GetSessionsAtTime(ctx, req.GetClusterId(), req.GetPageSize(), args)
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
	resps := &pgv1.GetSessionsAtTimeResponse{
		Sessions:            SessionsStateToGRPC(stats.Sessions),
		PreviousCollectTime: grpcapi.TimeToGRPC(stats.PreviousCollectTime),
		NextCollectTime:     grpcapi.TimeToGRPC(stats.NextCollectTime),
		NextPageToken:       api.PagingTokenToGRPC(nextPageToken),
	}
	return resps, err
}

func (pds *PerformanceDiagnosticsService) GetStatementsAtTime(ctx context.Context,
	req *pgv1.GetStatementsAtTimeRequest) (*pgv1.GetStatementsAtTimeResponse, error) {
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
	stats, more, err := pds.pg.GetStatementsAtTime(ctx, req.GetClusterId(), req.GetPageSize(), args)
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
	resps := &pgv1.GetStatementsAtTimeResponse{
		Statements:          StatementsAtTimeToGRPC(stats.Statements),
		PreviousCollectTime: grpcapi.TimeToGRPC(stats.PreviousCollectTime),
		NextCollectTime:     grpcapi.TimeToGRPC(stats.NextCollectTime),
		NextPageToken:       api.PagingTokenToGRPC(nextPageToken),
	}
	return resps, err
}

func (pds *PerformanceDiagnosticsService) GetStatementsDiff(ctx context.Context,
	req *pgv1.GetStatementsDiffRequest) (*pgv1.GetStatementsDiffResponse, error) {
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
	stats, more, err := pds.pg.GetStatementsDiff(ctx, req.GetClusterId(), req.GetPageSize(), args)
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
	resps := &pgv1.GetStatementsDiffResponse{
		DiffStatements: StatementsDiffToGRPC(stats.DiffStatements),
		NextPageToken:  api.PagingTokenToGRPC(nextPageToken),
	}
	return resps, err
}

func (pds *PerformanceDiagnosticsService) GetStatementsInterval(ctx context.Context,
	req *pgv1.GetStatementsIntervalRequest) (*pgv1.GetStatementsIntervalResponse, error) {
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
	stats, more, err := pds.pg.GetStatementsInterval(ctx, req.GetClusterId(), req.GetPageSize(), args)
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
	resps := &pgv1.GetStatementsIntervalResponse{
		Statements:    StatementsAtTimeToGRPC(stats.Statements),
		NextPageToken: api.PagingTokenToGRPC(nextPageToken),
	}
	return resps, err
}

func (pds *PerformanceDiagnosticsService) GetStatementsStats(ctx context.Context,
	req *pgv1.GetStatementsStatsRequest) (*pgv1.GetStatementsStatsResponse, error) {
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
	stats, more, err := pds.pg.GetStatementsStats(ctx, req.GetClusterId(), req.GetPageSize(), args)
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
	resps := &pgv1.GetStatementsStatsResponse{
		Statements:    StatementsStatsGRPC(stats.Statements),
		Query:         stats.Query,
		NextPageToken: api.PagingTokenToGRPC(nextPageToken),
	}
	return resps, err
}

func (pds *PerformanceDiagnosticsService) GetStatementStats(ctx context.Context,
	req *pgv1.GetStatementStatsRequest) (*pgv1.GetStatementStatsResponse, error) {
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
	stats, more, err := pds.pg.GetStatementStats(ctx, req.GetClusterId(), req.GetPageSize(), args)
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
	resps := &pgv1.GetStatementStatsResponse{
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
