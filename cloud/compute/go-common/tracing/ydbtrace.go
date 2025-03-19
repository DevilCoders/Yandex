// clone from http://bb.yandex-team.ru/projects/cloud/repos/compute/browse/go/common/pkg/ydbutil/trace.go?at=d41dc90ffe55e4a0c709c57cea60af07404b3913#47

package tracing

import (
	"fmt"

	"github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/ext"
	"github.com/opentracing/opentracing-go/log"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

var ydbTag = opentracing.Tag{Key: string(ext.Component), Value: "ydb"}

type YdbTrace struct {
	Driver  ydb.DriverTrace
	Client  table.ClientTrace
	Session table.SessionPoolTrace
}

type YDBTraceConfig struct {
	Driver      bool
	Client      bool
	SessionPool bool
}

func NewYDBTrace(cfg *YDBTraceConfig) YdbTrace {
	var opts YdbTrace

	if cfg.Driver {
		opts.Driver = ydb.DriverTrace{
			OnDial: func(info ydb.DialStartInfo) func(info ydb.DialDoneInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					span.LogFields(
						log.String("event", "DialStart"),
						log.String("address", info.Address),
					)
					WithError(span, nil)
				}
				return func(info ydb.DialDoneInfo) {
					if span := opentracing.SpanFromContext(info.Context); span != nil {
						span.LogFields(
							log.String("event", "DialDone"),
							log.String("address", info.Address),
						)
						WithError(span, info.Error)
					}
				}
			},
			OnGetConn: func(info ydb.GetConnStartInfo) func(info ydb.GetConnDoneInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					span.LogFields(
						log.String("event", "GetConnStart"),
					)
					WithError(span, nil)
				}
				return func(info ydb.GetConnDoneInfo) {
					if span := opentracing.SpanFromContext(info.Context); span != nil {
						span.LogFields(
							log.String("event", "GetConnDone"),
							log.String("address", info.Address),
						)
						WithError(span, info.Error)
					}
				}
			},
			OnDiscovery: func(info ydb.DiscoveryStartInfo) func(info ydb.DiscoveryDoneInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					span.LogFields(
						log.String("event", "DiscoveryStart"),
					)
					WithError(span, nil)
				}
				return func(info ydb.DiscoveryDoneInfo) {
					if span := opentracing.SpanFromContext(info.Context); span != nil {
						fields := make([]log.Field, 0, len(info.Endpoints)+1)
						fields = append(fields, log.String("event", "DiscoveryDone"))
						for _, e := range info.Endpoints {
							fields = append(fields, log.String("address", fmt.Sprintf("%s:%d", e.Addr, e.Port)))
						}
						span.LogFields(fields...)
						WithError(span, info.Error)
					}
				}
			},
			OnOperation: func(info ydb.OperationStartInfo) func(info ydb.OperationDoneInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					span.LogFields(
						log.String("event", "OperationStart"),
						log.String("address", info.Address),
						log.String("method", info.Method.Name()),
						log.String("service", info.Method.Service()),
						log.String("timeout", info.Params.Timeout.String()),
						log.String("cancelAfter", info.Params.CancelAfter.String()),
						log.String("mode", info.Params.Mode.String()),
					)
					WithError(span, nil)
				}
				return func(info ydb.OperationDoneInfo) {
					if span := opentracing.SpanFromContext(info.Context); span != nil {
						fields := []log.Field{
							log.String("event", "OperationDone"),
							log.String("operation", info.OpID),
							log.String("address", info.Address),
							log.String("method", info.Method.Name()),
							log.String("service", info.Method.Service()),
							log.String("timeout", info.Params.Timeout.String()),
							log.String("cancelAfter", info.Params.CancelAfter.String()),
							log.String("mode", info.Params.Mode.String()),
						}
						for _, i := range info.Issues {
							fields = append(fields, log.String("issue", i.GetMessage()))
						}
						span.LogFields(
							fields...,
						)
						WithError(span, info.Error)
					}
				}
			},
			OnOperationWait: func(info ydb.OperationWaitInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					span.LogFields(
						log.String("event", "OperationWait"),
						log.String("operation", info.OpID),
						log.String("address", info.Address),
						log.String("method", info.Method.Name()),
						log.String("service", info.Method.Service()),
						log.String("timeout", info.Params.Timeout.String()),
						log.String("cancelAfter", info.Params.CancelAfter.String()),
						log.String("mode", info.Params.Mode.String()),
					)
					WithError(span, nil)
				}
			},
		}
	}

	if cfg.Client {
		opts.Client = table.ClientTrace{

			OnCreateSession: func(info table.CreateSessionStartInfo) func(info table.CreateSessionDoneInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					span.LogFields(
						log.String("event", "CreateSessionStart"),
					)
					WithError(span, nil)
				}
				return func(info table.CreateSessionDoneInfo) {
					if span := opentracing.SpanFromContext(info.Context); span != nil {
						span.LogFields(
							log.String("event", "CreateSessionDone"),
							log.String("session", info.Session.ID),
						)
						WithError(span, info.Error)
					}
				}
			},

			OnKeepAlive: func(info table.KeepAliveStartInfo) func(info table.KeepAliveDoneInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					span.LogFields(
						log.String("event", "KeepAliveStart"),
						log.String("session", info.Session.ID),
					)
					WithError(span, nil)
				}
				return func(info table.KeepAliveDoneInfo) {
					if span := opentracing.SpanFromContext(info.Context); span != nil {
						span.LogFields(
							log.String("event", "KeepAliveDone"),
							log.String("session", info.Session.ID),
							log.String("status", info.SessionInfo.Status.String()),
						)
						WithError(span, info.Error)
					}
				}
			},

			OnDeleteSession: func(info table.DeleteSessionStartInfo) func(info table.DeleteSessionDoneInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					span.LogFields(
						log.String("event", "DeleteSessionStart"),
						log.String("session", info.Session.ID),
					)
					WithError(span, nil)
				}
				return func(info table.DeleteSessionDoneInfo) {
					if span := opentracing.SpanFromContext(info.Context); span != nil {
						span.LogFields(
							log.String("event", "DeleteSessionDone"),
							log.String("session", info.Session.ID),
						)
						WithError(span, info.Error)
					}
				}
			},

			OnPrepareDataQuery: func(info table.PrepareDataQueryStartInfo) func(info table.PrepareDataQueryDoneInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					// NOTE(frystile): should we log full query?
					span.LogFields(
						log.String("event", "PrepareDataQueryStart"),
						log.String("session", info.Session.ID),
					)
					WithError(span, nil)
				}
				return func(info table.PrepareDataQueryDoneInfo) {
					if span := opentracing.SpanFromContext(info.Context); span != nil {
						// NOTE(frystile): should we log full query?
						span.LogFields(
							log.String("event", "PrepareDataQueryDone"),
							log.String("session", info.Session.ID),
							log.Bool("cached", info.Cached),
						)
						WithError(span, info.Error)
					}
				}
			},

			OnExecuteDataQuery: func(info table.ExecuteDataQueryStartInfo) func(info table.ExecuteDataQueryDoneInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					// NOTE(frystile): should we log full query?
					span.LogFields(
						log.String("event", "ExecuteDataQueryStart"),
						log.String("session", info.Session.ID),
						log.String("txID", info.TxID),
					)
					WithError(span, nil)
				}
				return func(info table.ExecuteDataQueryDoneInfo) {
					if span := opentracing.SpanFromContext(info.Context); span != nil {
						// NOTE(frystile): should we log full query?
						span.LogFields(
							log.String("event", "ExecuteDataQueryDone"),
							log.String("session", info.Session.ID),
							log.String("txID", info.TxID),
							log.Bool("prepared", info.Prepared),
						)
						WithError(span, info.Error)
					}
				}
			},
			OnBeginTransaction: func(info table.BeginTransactionStartInfo) func(info table.BeginTransactionDoneInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					span.LogFields(
						log.String("event", "BeginTransaction"),
						log.String("session", info.Session.ID),
					)
					WithError(span, nil)
				}
				return func(info table.BeginTransactionDoneInfo) {
					if span := opentracing.SpanFromContext(info.Context); span != nil {
						span.LogFields(
							log.String("event", "BeginTransactionDone"),
							log.String("session", info.Session.ID),
							log.String("txID", info.TxID),
						)
						WithError(span, info.Error)
					}
				}
			},
			OnCommitTransaction: func(info table.CommitTransactionStartInfo) func(info table.CommitTransactionDoneInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					span.LogFields(
						log.String("event", "CommitTransactionStart"),
						log.String("session", info.Session.ID),
						log.String("txID", info.TxID),
					)
					WithError(span, nil)
				}
				return func(info table.CommitTransactionDoneInfo) {
					if span := opentracing.SpanFromContext(info.Context); span != nil {
						span.LogFields(
							log.String("event", "CommitTransactionDone"),
							log.String("session", info.Session.ID),
							log.String("txID", info.TxID),
						)
						WithError(span, info.Error)
					}
				}
			},

			OnRollbackTransaction: func(info table.RollbackTransactionStartInfo) func(info table.RollbackTransactionDoneInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					span.LogFields(
						log.String("event", "RollbackTransactionStart"),
						log.String("session", info.Session.ID),
						log.String("txID", info.TxID),
					)
					WithError(span, nil)
				}
				return func(info table.RollbackTransactionDoneInfo) {
					if span := opentracing.SpanFromContext(info.Context); span != nil {
						span.LogFields(
							log.String("event", "RollbackTransactionDone"),
							log.String("session", info.Session.ID),
							log.String("txID", info.TxID),
						)
						WithError(span, info.Error)
					}
				}
			},
		}
	}

	if cfg.SessionPool {
		opts.Session = table.SessionPoolTrace{
			OnGet: func(info table.SessionPoolGetStartInfo) func(info table.SessionPoolGetDoneInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					span.LogFields(
						log.String("event", "SessionPoolGetStart"),
					)
					WithError(span, nil)
				}
				return func(info table.SessionPoolGetDoneInfo) {
					if span := opentracing.SpanFromContext(info.Context); span != nil {
						span.LogFields(
							log.String("event", "SessionPoolGetDone"),
							log.String("session", info.Session.ID),
						)
						WithError(span, info.Error)
					}
				}
			},
			OnTake: func(info table.SessionPoolTakeStartInfo) func(info table.SessionPoolTakeDoneInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					span.LogFields(
						log.String("event", "SessionPoolTakeStart"),
						log.String("session", info.Session.ID),
					)
					WithError(span, nil)
				}
				return func(info table.SessionPoolTakeDoneInfo) {
					if span := opentracing.SpanFromContext(info.Context); span != nil {
						span.LogFields(
							log.String("event", "SessionPoolTakeDone"),
							log.String("session", info.Session.ID),
							log.Bool("took", info.Took),
						)
						WithError(span, info.Error)
					}
				}
			},
			OnPut: func(info table.SessionPoolPutStartInfo) func(info table.SessionPoolPutDoneInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					span.LogFields(
						log.String("event", "SessionPoolPutStart"),
						log.String("session", info.Session.ID),
					)
					WithError(span, nil)
				}
				return func(info table.SessionPoolPutDoneInfo) {
					if span := opentracing.SpanFromContext(info.Context); span != nil {
						span.LogFields(
							log.String("event", "SessionPoolPutDone"),
							log.String("session", info.Session.ID),
						)
						WithError(span, info.Error)
					}
				}
			},
			OnClose: func(info table.SessionPoolCloseStartInfo) func(info table.SessionPoolCloseDoneInfo) {
				if span := opentracing.SpanFromContext(info.Context); span != nil {
					span.LogFields(
						log.String("event", "SessionPoolCloseStart"),
					)
					WithError(span, nil)
				}
				return func(info table.SessionPoolCloseDoneInfo) {
					if span := opentracing.SpanFromContext(info.Context); span != nil {
						span.LogFields(
							log.String("event", "SessionPoolCloseDone"),
						)
						WithError(span, info.Error)
					}
				}
			},
		}
	}

	return opts
}
