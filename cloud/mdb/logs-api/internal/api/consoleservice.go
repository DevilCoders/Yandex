package api

import (
	"context"

	apiv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/console/v1"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/models"
)

type ConsoleLogsService struct {
	apiv1.UnimplementedLogServiceServer

	logs logic.Logs
}

var _ apiv1.LogServiceServer = &ConsoleLogsService{}

func NewConsoleLogsService(logs logic.Logs) *ConsoleLogsService {
	return &ConsoleLogsService{logs: logs}
}

func (ls *ConsoleLogsService) ListLogs(ctx context.Context, req *apiv1.ListLogsRequest) (*apiv1.ListLogsResponse, error) {
	var err error
	var criteria models.Criteria
	var filterString string

	switch sel := req.Selector.(type) {
	case *apiv1.ListLogsRequest_Paging:
		if criteria, filterString, err = criteriaFromPaging(sel.Paging); err != nil {
			return nil, semerr.InvalidInput("invalid page token")
		}
	case *apiv1.ListLogsRequest_Criteria:
		if criteria, filterString, err = criteriaFromGRPC(sel.Criteria); err != nil {
			return nil, err
		}
	}

	logs, more, offset, err := ls.logs.ListLogs(ctx, criteria)
	if err != nil {
		return nil, err
	}

	token, err := buildNextPage(criteria, filterString, offset)
	if err != nil {
		return nil, err
	}

	if !more {
		token = nil
	}

	return &apiv1.ListLogsResponse{
		Records:  logsToGRPC(logs),
		NextPage: token,
	}, nil
}
