package grpc

import (
	"context"

	"github.com/golang/protobuf/ptypes/timestamp"

	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// ClusterService implements common methods for all DBs
type ClusterService struct {
	L    log.Logger
	Logs common.Logs

	Config api.Config
}

type ListClusterLogsRequest interface {
	GetClusterId() string
	GetColumnFilter() []string
	GetFilter() string
	GetFromTime() *timestamp.Timestamp
	GetToTime() *timestamp.Timestamp
	GetPageSize() int64
	GetPageToken() string
	GetAlwaysNextPageToken() bool
}

// ListLogs has DB-agnostic functionality
func (cs *ClusterService) ListLogs(
	ctx context.Context,
	st logs.ServiceType,
	req ListClusterLogsRequest,
) (logs []logs.Message, nextPageToken int64, more bool, err error) {
	var opts common.LogsOptions

	opts.ColumnFilter = req.GetColumnFilter()

	if req.GetFromTime() != nil {
		opts.FromTS.Set(TimeFromGRPC(req.GetFromTime()))
	}
	if req.GetToTime() != nil {
		opts.ToTS.Set(TimeFromGRPC(req.GetToTime()))
	}

	offset, ok, err := api.PageTokenFromGRPC(req.GetPageToken())
	if err != nil {
		return nil, 0, false, err
	}
	if ok {
		opts.Offset.Set(offset)
	}

	flt, err := FilterFromGRPC(req.GetFilter())
	if err != nil {
		return nil, 0, false, err
	}
	opts.Filter = flt

	logs, more, err = cs.Logs.Logs(ctx, req.GetClusterId(), st, req.GetPageSize(), opts)
	if err != nil {
		return nil, 0, false, err
	}

	if len(logs) > 0 {
		nextPageToken = logs[len(logs)-1].NextMessageToken
	} else {
		// If we retrieved zero logs, we have to return 'good' token. That token will be whatever
		// we received from caller.
		nextPageToken = offset
	}

	return logs, nextPageToken, more, nil
}

type StreamClusterLogsRequest interface {
	GetClusterId() string
	GetColumnFilter() []string
	GetFilter() string
	GetFromTime() *timestamp.Timestamp
	GetToTime() *timestamp.Timestamp
	GetRecordToken() string
}

type LogSender func(l logs.Message) error

// ListLogsStream has DB-agnostic functionality
func (cs *ClusterService) StreamLogs(ctx context.Context, lst logs.ServiceType, req StreamClusterLogsRequest, sender LogSender) error {
	// Set options
	var opts common.LogsOptions

	opts.ColumnFilter = req.GetColumnFilter()

	if req.GetFromTime() != nil {
		opts.FromTS.Set(TimeFromGRPC(req.GetFromTime()))
	}
	if req.GetToTime() != nil {
		opts.ToTS.Set(TimeFromGRPC(req.GetToTime()))
	}

	offset, ok, err := api.PageTokenFromGRPC(req.GetRecordToken())
	if err != nil {
		return err
	}
	if ok {
		opts.Offset.Set(offset)
	}

	flt, err := FilterFromGRPC(req.GetFilter())
	if err != nil {
		return err
	}
	opts.Filter = flt

	ctx, cancel := context.WithCancel(ctx)

	// Initiate streaming
	ch, err := cs.Logs.Stream(ctx, req.GetClusterId(), lst, opts)
	if err != nil {
		cancel()
		return err
	}
	// Panic protection
	defer func() {
		cancel()
		for range ch {
		}
	}()

	// Retrieve results
	for batch := range ch {
		if batch.Err != nil {
			// Closing channel after batch error is part of logs API. We want to report bad behavior so
			// we can't rely on panic protection to cancel and drain channel for us.
			//
			// Insolent bugs!
			cancel()
			for range ch {
				msg := "another batch of logs after batch error"
				cs.L.Error(msg)
				sentry.GlobalClient().CaptureError(ctx, xerrors.New(msg), nil)
			}
			return batch.Err
		}

		// Send results
		for _, l := range batch.Logs {
			if err := sender(l); err != nil {
				// We do not need to cancel or drain channel here, panic protection will do it for us
				return err
			}
		}
	}

	return nil
}
