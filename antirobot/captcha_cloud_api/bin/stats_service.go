package main

import (
	"context"

	pb "a.yandex-team.ru/antirobot/captcha_cloud_api/proto"
	smartcaptcha_pb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/smartcaptcha/v1"
	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	"github.com/ydb-platform/ydb-go-sdk/v3/table"
	"github.com/ydb-platform/ydb-go-sdk/v3/table/result"
	"github.com/ydb-platform/ydb-go-sdk/v3/table/types"
)

type StatsServer struct {
	smartcaptcha_pb.UnimplementedStatsServiceServer
	Server
}

func (server *StatsServer) folderInternal(ctx context.Context, in *smartcaptcha_pb.GetFolderStatsRequest) (*smartcaptcha_pb.FolderStats, error) {
	if _, err := server.Authorize(ctx, cloudauth.ResourceFolder(in.FolderId), CaptchaGetPermission); err != nil {
		return nil, err
	}

	readTx := table.TxControl(table.BeginTx(table.WithOnlineReadOnly()), table.CommitTx())
	var (
		res result.Result
		err error
	)

	err = (*server.YdbConnection).Table().Do(
		ctx,
		func(ctx context.Context, s table.Session) (err error) {
			_, res, err = s.Execute(
				ctx,
				readTx,
				`-- select
				declare $folder_id as String;

				select
					CAST(count(*) as Int64) as cnt,
					folder_id
				from `+"`captcha-settings`"+`
				where folder_id == $folder_id
				group by folder_id
				`,
				table.NewQueryParameters(
					table.ValueParam("$folder_id", types.StringValue([]byte(in.FolderId))),
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

	var captchasUsage int64 = 0
	for res.NextResultSet(ctx, "cnt", "folder_id") {
		for res.NextRow() {
			var (
				cnt      int64
				folderID string
			)
			err = res.ScanWithDefaults(&cnt, &folderID)
			if err != nil {
				return nil, err
			}
			captchasUsage = cnt
		}
	}

	return &smartcaptcha_pb.FolderStats{
		TotalInstances: captchasUsage,
	}, nil
}

func (server *StatsServer) Folder(ctx context.Context, in *smartcaptcha_pb.GetFolderStatsRequest) (*smartcaptcha_pb.FolderStats, error) {
	result, err := server.folderInternal(ctx, in)
	server.AccessLogger.Log(&pb.TLogRecord{StatsServerFolderRecord: &pb.TStatsServerFolderRecord{Error: ErrorToProto(err), Request: in, Response: result}})
	return result, err
}
