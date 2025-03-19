package api

import (
	"context"

	apiv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/logs/v1"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type LogsService struct {
	apiv1.UnimplementedLogServiceServer

	logs logic.Logs
	l    log.Logger
}

var _ apiv1.LogServiceServer = &LogsService{}

func NewLogsService(logs logic.Logs, l log.Logger) *LogsService {
	return &LogsService{logs: logs, l: l}
}

func (ls *LogsService) Read(req *apiv1.ReadLogsRequest, stream apiv1.LogService_ReadServer) error {
	var err error
	var criteria models.Criteria
	var filterString string

	switch sel := req.Selector.(type) {
	case *apiv1.ReadLogsRequest_Paging:
		if criteria, filterString, err = criteriaFromPaging(sel.Paging); err != nil {
			return semerr.InvalidInput("invalid page token")
		}
	case *apiv1.ReadLogsRequest_Criteria:
		if criteria, filterString, err = criteriaFromGRPC(sel.Criteria); err != nil {
			return err
		}
	}

	ctx, cancel := context.WithCancel(stream.Context())

	logsChannel, err := ls.logs.StreamLogs(ctx, criteria)
	if err != nil {
		cancel()
		return err
	}

	// Panic protection
	defer func() {
		cancel()
		for range logsChannel {
		}
	}()

	// Retrieve results
	for batch := range logsChannel {
		if batch.Err != nil {
			// Closing channel after batch error is part of logs API. We want to report bad behavior so
			// we can't rely on panic protection to cancel and drain channel for us.
			//
			// Insolent bugs!
			cancel()
			for range logsChannel {
				msg := "another batch of logs after batch error"
				ls.l.Error(msg)
				sentry.GlobalClient().CaptureError(ctx, xerrors.New(msg), nil)
			}
			return batch.Err
		}

		// Send results
		for _, l := range batch.Logs {
			token, err := buildNextPage(criteria, filterString, l.Offset)
			if err != nil {
				return err
			}

			if err := stream.Send(&apiv1.ReadLogRecord{
				Record:   logToGRPC(l),
				NextPage: token,
			}); err != nil {
				// We do not need to cancel or drain channel here, panic protection will do it for us
				return err
			}
		}
	}

	return nil
}
