package main

import (
	"context"
	"fmt"

	pb "a.yandex-team.ru/antirobot/captcha_cloud_api/proto"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/quota"
	smartcaptcha_pb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/smartcaptcha/v1"
	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	"github.com/ydb-platform/ydb-go-sdk/v3/table"
	"github.com/ydb-platform/ydb-go-sdk/v3/table/result"
	"github.com/ydb-platform/ydb-go-sdk/v3/table/types"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/types/known/emptypb"
)

type QuotaServer struct {
	smartcaptcha_pb.UnimplementedQuotaServiceServer
	Server
}

func (server *QuotaServer) getDefaultInternal(ctx context.Context, in *quota.GetQuotaDefaultRequest) (*quota.GetQuotaDefaultResponse, error) {
	return &quota.GetQuotaDefaultResponse{
		Metrics: []*quota.MetricLimit{
			{Name: string(QuotaMetricCaptchasCount), Limit: server.Args.DefaultCaptchasQuota},
		},
	}, nil
}

func (server *QuotaServer) GetDefault(ctx context.Context, in *quota.GetQuotaDefaultRequest) (*quota.GetQuotaDefaultResponse, error) {
	result, err := server.getDefaultInternal(ctx, in)
	server.AccessLogger.Log(&pb.TLogRecord{QuotaServerGetDefaultRecord: &pb.TQuotaServerGetDefaultRecord{Error: ErrorToProto(err), Request: in, Response: result}})
	return result, err
}

func (server *QuotaServer) updateMetricInternal(ctx context.Context, in *quota.UpdateQuotaMetricRequest) (*emptypb.Empty, error) {
	if _, err := server.Authorize(ctx, cloudauth.ResourceCloud(in.CloudId), QuotaUpdatePermission); err != nil {
		return nil, err
	}

	validMetrics := map[string]bool{
		string(QuotaMetricCaptchasCount): true,
	}
	if !validMetrics[in.Metric.Name] {
		return nil, status.Errorf(codes.InvalidArgument, fmt.Sprintf("Invalig metric name '%v'", in.Metric.Name))
	}

	writeTx := table.TxControl(table.BeginTx(table.WithSerializableReadWrite()), table.CommitTx())
	var res result.Result
	err := (*server.YdbConnection).Table().Do(
		ctx,
		func(ctx context.Context, s table.Session) (err error) {
			_, res, err = s.Execute(
				ctx,
				writeTx,
				`-- Update metric
				declare $cloud_id as String;
				declare $metric as String;
				declare $limit as Int64;

				upsert into `+"`cloud-quotas`"+` (cloud_id,  metric,  lim)
				values                               ($cloud_id, $metric, $limit)
				`,
				table.NewQueryParameters(
					table.ValueParam("$cloud_id", types.StringValue([]byte(in.CloudId))),
					table.ValueParam("$metric", types.StringValue([]byte(in.Metric.Name))),
					table.ValueParam("$limit", types.Int64Value(in.Metric.Limit)),
				),
			)
			return err
		},
		table.WithIdempotent(),
	)
	if err != nil {
		return nil, err
	}
	if res.Err() != nil {
		return nil, res.Err()
	}
	return &emptypb.Empty{}, nil
}

func (server *QuotaServer) UpdateMetric(ctx context.Context, in *quota.UpdateQuotaMetricRequest) (*emptypb.Empty, error) {
	result, err := server.updateMetricInternal(ctx, in)
	server.AccessLogger.Log(&pb.TLogRecord{QuotaServerUpdateMetricRecord: &pb.TQuotaServerUpdateMetricRecord{Error: ErrorToProto(err), Request: in, Response: result}})
	return result, err
}

func (server *QuotaServer) batchUpdateMetricInternal(ctx context.Context, in *quota.BatchUpdateQuotaMetricsRequest) (*emptypb.Empty, error) {
	if _, err := server.Authorize(ctx, cloudauth.ResourceCloud(in.CloudId), QuotaUpdatePermission); err != nil {
		return nil, err
	}

	for _, metric := range in.Metrics {
		_, err := server.UpdateMetric(ctx, &quota.UpdateQuotaMetricRequest{
			CloudId: in.CloudId,
			Metric:  metric,
		})
		if err != nil {
			return nil, err
		}
	}
	return &emptypb.Empty{}, nil
}

func (server *QuotaServer) BatchUpdateMetric(ctx context.Context, in *quota.BatchUpdateQuotaMetricsRequest) (*emptypb.Empty, error) {
	result, err := server.batchUpdateMetricInternal(ctx, in)
	server.AccessLogger.Log(&pb.TLogRecord{QuotaServerBatchUpdateMetricRecord: &pb.TQuotaServerBatchUpdateMetricRecord{Error: ErrorToProto(err), Request: in, Response: result}})
	return result, err
}

func (server *QuotaServer) getInternal(ctx context.Context, in *quota.GetQuotaRequest) (*quota.Quota, error) {
	if _, err := server.Authorize(ctx, cloudauth.ResourceCloud(in.CloudId), QuotaGetPermission); err != nil {
		return nil, err
	}

	var (
		res           result.Result
		captchasUsage int64 = 0
		err           error
	)
	err = (*server.YdbConnection).Table().Do(
		ctx,
		func(ctx context.Context, s table.Session) (err error) {
			tx, errTx := s.BeginTransaction(ctx, table.TxSettings(table.WithSerializableReadWrite())) // TODO: как сделать read-only транзакцию?
			if errTx != nil {
				return errTx
			}
			defer func() {
				_ = tx.Rollback(ctx)
			}()

			res, err = tx.Execute(
				ctx,
				`-- count quota usage
				declare $cloud_id as String;

				select
					CAST(count(*) as Int64) as cnt,
					cloud_id
				from `+"`captcha-settings`"+`
				where cloud_id == $cloud_id
				group by cloud_id
				`,
				table.NewQueryParameters(
					table.ValueParam("$cloud_id", types.StringValue([]byte(in.CloudId))),
				),
			)
			if err != nil {
				return err
			}
			if res.Err() != nil {
				return res.Err()
			}
			for res.NextResultSet(ctx, "cnt", "cloud_id") {
				for res.NextRow() {
					var (
						cnt     int64
						cloidID string
					)
					err = res.ScanWithDefaults(&cnt, &cloidID)
					if err != nil {
						return err
					}
					captchasUsage = cnt
				}
			}

			res, err = tx.Execute(
				ctx,
				`-- count quota usage
				declare $cloud_id as String;
				select
					cloud_id,
					metric,
					usage,
					lim
				from `+"`cloud-quotas`"+`
				where cloud_id = $cloud_id
				`,
				table.NewQueryParameters(
					table.ValueParam("$cloud_id", types.StringValue([]byte(in.CloudId))),
				),
			)
			if err != nil {
				return err
			}

			_, err = tx.CommitTx(ctx)
			return err
		},
		table.WithIdempotent(),
	)
	if err != nil {
		return nil, err
	}
	if res.Err() != nil {
		return nil, res.Err()
	}

	usages := map[string]*quota.QuotaMetric{
		string(QuotaMetricCaptchasCount): &quota.QuotaMetric{
			Name:  string(QuotaMetricCaptchasCount),
			Limit: server.Args.DefaultCaptchasQuota,
			Usage: float64(captchasUsage),
		},
	}

	for res.NextResultSet(ctx, "cloud_id", "metric", "usage", "lim") {
		for res.NextRow() {
			var (
				cloudID string
				metric  string
				usage   float64
				limit   int64
			)
			err = res.ScanWithDefaults(&cloudID, &metric, &usage, &limit)
			if err != nil {
				return nil, err
			}
			if val, exists := usages[metric]; exists {
				val.Limit = limit
				if metric != string(QuotaMetricCaptchasCount) {
					val.Usage = usage
				}
			}
		}
	}

	values := make([]*quota.QuotaMetric, 0, len(usages))
	for _, v := range usages {
		values = append(values, v)
	}

	return &quota.Quota{
		CloudId: in.CloudId,
		Metrics: values,
	}, nil
}

func (server *QuotaServer) Get(ctx context.Context, in *quota.GetQuotaRequest) (*quota.Quota, error) {
	result, err := server.getInternal(ctx, in)
	server.AccessLogger.Log(&pb.TLogRecord{QuotaServerGetRecord: &pb.TQuotaServerGetRecord{Error: ErrorToProto(err), Request: in, Response: result}})
	return result, err
}
