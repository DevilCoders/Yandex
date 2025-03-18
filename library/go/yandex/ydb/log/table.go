package log

import (
	"time"

	"github.com/ydb-platform/ydb-go-sdk/v3"
	"github.com/ydb-platform/ydb-go-sdk/v3/retry"
	"github.com/ydb-platform/ydb-go-sdk/v3/trace"

	"a.yandex-team.ru/library/go/core/log"
)

// Table makes trace.Table with zap logging
func Table(l log.Logger, details trace.Details) (t trace.Table) {
	l = l.WithName("ydb").WithName("table")
	if details&trace.TableEvents != 0 {
		t.OnInit = func(info trace.TableInitStartInfo) func(trace.TableInitDoneInfo) {
			l.Info("initializing",
				log.String("version", version),
			)
			start := time.Now()
			return func(info trace.TableInitDoneInfo) {
				l.Info("initialized",
					log.String("version", version),
					log.Duration("latency", time.Since(start)),
					log.Int("minSize", info.KeepAliveMinSize),
					log.Int("maxSize", info.Limit),
				)
			}
		}
		t.OnClose = func(info trace.TableCloseStartInfo) func(trace.TableCloseDoneInfo) {
			l.Info("closing",
				log.String("version", version),
			)
			start := time.Now()
			return func(info trace.TableCloseDoneInfo) {
				if info.Error != nil {
					l.Info("closed",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
					)
				} else {
					l.Error("close failed",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.Error(info.Error),
					)
				}
			}
		}
		do := l.WithName("do")
		doTx := l.WithName("doTx")
		createSession := l.WithName("createSession")
		t.OnCreateSession = func(info trace.TableCreateSessionStartInfo) func(info trace.TableCreateSessionIntermediateInfo) func(trace.TableCreateSessionDoneInfo) {
			createSession.Debug("init",
				log.String("version", version),
			)
			start := time.Now()
			return func(info trace.TableCreateSessionIntermediateInfo) func(trace.TableCreateSessionDoneInfo) {
				if info.Error == nil {
					createSession.Debug("intermediate",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
					)
				} else {
					createSession.Warn("intermediate",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.Error(info.Error),
					)
				}
				return func(info trace.TableCreateSessionDoneInfo) {
					if info.Error == nil {
						createSession.Debug("finish",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.Int("attempts", info.Attempts),
							log.String("id", info.Session.ID()),
							log.String("status", info.Session.Status()),
						)
					} else {
						createSession.Error("finish",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.Int("attempts", info.Attempts),
							log.Error(info.Error),
						)
					}
				}
			}
		}
		t.OnDo = func(info trace.TableDoStartInfo) func(info trace.TableDoIntermediateInfo) func(trace.TableDoDoneInfo) {
			idempotent := info.Idempotent
			do.Debug("init",
				log.String("version", version),
				log.Bool("idempotent", idempotent))
			start := time.Now()
			return func(info trace.TableDoIntermediateInfo) func(trace.TableDoDoneInfo) {
				if info.Error == nil {
					do.Debug("intermediate",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.Bool("idempotent", idempotent),
					)
				} else {
					f := do.Warn
					if !ydb.IsYdbError(info.Error) {
						f = do.Debug
					}
					m := retry.Check(info.Error)
					f("intermediate",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.Bool("idempotent", idempotent),
						log.Bool("retryable", m.MustRetry(idempotent)),
						log.Bool("deleteSession", m.MustDeleteSession()),
						log.Int64("code", m.StatusCode()),
						log.Error(info.Error),
					)
				}
				return func(info trace.TableDoDoneInfo) {
					if info.Error == nil {
						do.Debug("finish",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.Bool("idempotent", idempotent),
							log.Int("attempts", info.Attempts),
						)
					} else {
						f := do.Error
						if !ydb.IsYdbError(info.Error) {
							f = do.Debug
						}
						m := retry.Check(info.Error)
						f("finish",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.Bool("idempotent", idempotent),
							log.Bool("retryable", m.MustRetry(idempotent)),
							log.Bool("deleteSession", m.MustDeleteSession()),
							log.Int64("code", m.StatusCode()),
							log.Int("attempts", info.Attempts),
							log.Error(info.Error),
						)
					}
				}
			}
		}
		t.OnDoTx = func(info trace.TableDoTxStartInfo) func(info trace.TableDoTxIntermediateInfo) func(trace.TableDoTxDoneInfo) {
			idempotent := info.Idempotent
			doTx.Debug("init",
				log.String("version", version),
				log.Bool("idempotent", idempotent))
			start := time.Now()
			return func(info trace.TableDoTxIntermediateInfo) func(trace.TableDoTxDoneInfo) {
				if info.Error == nil {
					doTx.Debug("intermediate",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.Bool("idempotent", idempotent),
					)
				} else {
					f := doTx.Warn
					if !ydb.IsYdbError(info.Error) {
						f = doTx.Debug
					}
					m := retry.Check(info.Error)
					f("intermediate",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.Bool("idempotent", idempotent),
						log.Bool("retryable", m.MustRetry(idempotent)),
						log.Bool("deleteSession", m.MustDeleteSession()),
						log.Int64("code", m.StatusCode()),
						log.Error(info.Error),
					)
				}
				return func(info trace.TableDoTxDoneInfo) {
					if info.Error == nil {
						doTx.Debug("finish",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.Bool("idempotent", idempotent),
							log.Int("attempts", info.Attempts),
						)
					} else {
						f := doTx.Warn
						if !ydb.IsYdbError(info.Error) {
							f = doTx.Debug
						}
						m := retry.Check(info.Error)
						f("finish",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.Bool("idempotent", idempotent),
							log.Int("attempts", info.Attempts),
							log.Bool("retryable", m.MustRetry(idempotent)),
							log.Bool("deleteSession", m.MustDeleteSession()),
							log.Int64("code", m.StatusCode()),
							log.Error(info.Error),
						)
					}
				}
			}
		}
	}
	if details&trace.TableSessionEvents != 0 {
		l := l.WithName("session")
		if details&trace.TableSessionLifeCycleEvents != 0 {
			t.OnSessionNew = func(info trace.TableSessionNewStartInfo) func(trace.TableSessionNewDoneInfo) {
				l.Debug("try to create",
					log.String("version", version),
				)
				start := time.Now()
				return func(info trace.TableSessionNewDoneInfo) {
					if info.Error == nil {
						l.Info("created",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.String("id", info.Session.ID()),
						)
					} else {
						l.Error("create failed",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.Error(info.Error),
						)
					}
				}
			}
			t.OnSessionDelete = func(info trace.TableSessionDeleteStartInfo) func(trace.TableSessionDeleteDoneInfo) {
				session := info.Session
				l.Debug("try to delete",
					log.String("version", version),
					log.String("id", session.ID()),
					log.String("status", session.Status()),
				)
				start := time.Now()
				return func(info trace.TableSessionDeleteDoneInfo) {
					if info.Error == nil {
						l.Debug("deleted",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.String("id", session.ID()),
							log.String("status", session.Status()),
						)
					} else {
						l.Warn("delete failed",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.String("id", session.ID()),
							log.String("status", session.Status()),
							log.Error(info.Error),
						)
					}
				}
			}
			t.OnSessionKeepAlive = func(info trace.TableKeepAliveStartInfo) func(trace.TableKeepAliveDoneInfo) {
				session := info.Session
				l.Debug("keep-aliving",
					log.String("version", version),
					log.String("id", session.ID()),
					log.String("status", session.Status()),
				)
				start := time.Now()
				return func(info trace.TableKeepAliveDoneInfo) {
					if info.Error == nil {
						l.Debug("keep-alived",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.String("id", session.ID()),
							log.String("status", session.Status()),
						)
					} else {
						l.Warn("keep-alive failed",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.String("id", session.ID()),
							log.String("status", session.Status()),
							log.Error(info.Error),
						)
					}
				}
			}
		}
		if details&trace.TableSessionQueryEvents != 0 {
			l := l.WithName("query")
			if details&trace.TableSessionQueryInvokeEvents != 0 {
				l := l.WithName("invoke")
				t.OnSessionQueryPrepare = func(
					info trace.TablePrepareDataQueryStartInfo,
				) func(
					trace.TablePrepareDataQueryDoneInfo,
				) {
					session := info.Session
					query := info.Query
					l.Debug("preparing",
						log.String("version", version),
						log.String("id", session.ID()),
						log.String("status", session.Status()),
						log.String("query", query),
					)
					start := time.Now()
					return func(info trace.TablePrepareDataQueryDoneInfo) {
						if info.Error == nil {
							l.Debug(
								"prepared",
								log.String("version", version),
								log.Duration("latency", time.Since(start)),
								log.String("id", session.ID()),
								log.String("status", session.Status()),
								log.String("query", query),
								log.String("yql", info.Result.String()),
							)
						} else {
							l.Error(
								"prepare failed",
								log.String("version", version),
								log.Duration("latency", time.Since(start)),
								log.String("id", session.ID()),
								log.String("status", session.Status()),
								log.String("query", query),
								log.Error(info.Error),
							)
						}
					}
				}
				t.OnSessionQueryExecute = func(
					info trace.TableExecuteDataQueryStartInfo,
				) func(
					trace.TableExecuteDataQueryDoneInfo,
				) {
					session := info.Session
					query := info.Query
					params := info.Parameters
					l.Debug("executing",
						log.String("version", version),
						log.String("id", session.ID()),
						log.String("status", session.Status()),
						log.String("yql", query.String()),
						log.String("params", params.String()),
					)
					start := time.Now()
					return func(info trace.TableExecuteDataQueryDoneInfo) {
						if info.Error == nil {
							tx := info.Tx
							l.Debug("executed",
								log.String("version", version),
								log.Duration("latency", time.Since(start)),
								log.String("id", session.ID()),
								log.String("status", session.Status()),
								log.String("tx", tx.ID()),
								log.String("yql", query.String()),
								log.String("params", params.String()),
								log.Bool("prepared", info.Prepared),
								log.NamedError("resultErr", info.Result.Err()),
							)
						} else {
							l.Error("execute failed",
								log.String("version", version),
								log.Duration("latency", time.Since(start)),
								log.String("id", session.ID()),
								log.String("status", session.Status()),
								log.String("yql", query.String()),
								log.String("params", params.String()),
								log.Bool("prepared", info.Prepared),
								log.Error(info.Error),
							)
						}
					}
				}
			}
			if details&trace.TableSessionQueryStreamEvents != 0 {
				l := l.WithName("stream")
				t.OnSessionQueryStreamExecute = func(
					info trace.TableSessionQueryStreamExecuteStartInfo,
				) func(
					intermediateInfo trace.TableSessionQueryStreamExecuteIntermediateInfo,
				) func(
					trace.TableSessionQueryStreamExecuteDoneInfo,
				) {
					session := info.Session
					query := info.Query
					params := info.Parameters
					l.Debug("executing",
						log.String("version", version),
						log.String("id", session.ID()),
						log.String("status", session.Status()),
						log.String("yql", query.String()),
						log.String("params", params.String()),
					)
					start := time.Now()
					return func(
						info trace.TableSessionQueryStreamExecuteIntermediateInfo,
					) func(
						trace.TableSessionQueryStreamExecuteDoneInfo,
					) {
						if info.Error == nil {
							l.Debug(`intermediate`)
						} else {
							l.Error(`intermediate failed`,
								log.Error(info.Error),
							)
						}
						return func(info trace.TableSessionQueryStreamExecuteDoneInfo) {
							if info.Error == nil {
								l.Debug("executed",
									log.String("version", version),
									log.Duration("latency", time.Since(start)),
									log.String("id", session.ID()),
									log.String("status", session.Status()),
									log.String("yql", query.String()),
									log.String("params", params.String()),
									log.Error(info.Error),
								)
							} else {
								l.Error("execute failed",
									log.String("version", version),
									log.Duration("latency", time.Since(start)),
									log.String("id", session.ID()),
									log.String("status", session.Status()),
									log.String("yql", query.String()),
									log.String("params", params.String()),
									log.Error(info.Error),
								)
							}
						}
					}
				}
				t.OnSessionQueryStreamRead = func(
					info trace.TableSessionQueryStreamReadStartInfo,
				) func(
					trace.TableSessionQueryStreamReadIntermediateInfo,
				) func(
					trace.TableSessionQueryStreamReadDoneInfo,
				) {
					session := info.Session
					l.Debug("reading",
						log.String("version", version),
						log.String("id", session.ID()),
						log.String("status", session.Status()),
					)
					start := time.Now()
					return func(
						info trace.TableSessionQueryStreamReadIntermediateInfo,
					) func(
						trace.TableSessionQueryStreamReadDoneInfo,
					) {
						if info.Error == nil {
							l.Debug(`intermediate`)
						} else {
							l.Error(`intermediate failed`,
								log.Error(info.Error),
							)
						}
						return func(info trace.TableSessionQueryStreamReadDoneInfo) {
							if info.Error == nil {
								l.Debug("read",
									log.String("version", version),
									log.Duration("latency", time.Since(start)),
									log.String("id", session.ID()),
									log.String("status", session.Status()),
								)
							} else {
								l.Error("read failed",
									log.String("version", version),
									log.Duration("latency", time.Since(start)),
									log.String("id", session.ID()),
									log.String("status", session.Status()),
									log.Error(info.Error),
								)
							}
						}
					}
				}
			}
		}
		if details&trace.TableSessionTransactionEvents != 0 {
			l := l.WithName("transaction")
			t.OnSessionTransactionBegin = func(info trace.TableSessionTransactionBeginStartInfo) func(trace.TableSessionTransactionBeginDoneInfo) {
				session := info.Session
				l.Debug("beginning",
					log.String("version", version),
					log.String("id", session.ID()),
					log.String("status", session.Status()),
				)
				start := time.Now()
				return func(info trace.TableSessionTransactionBeginDoneInfo) {
					if info.Error == nil {
						l.Debug("began",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.String("id", session.ID()),
							log.String("status", session.Status()),
							log.String("tx", info.Tx.ID()),
						)
					} else {
						l.Debug("begin failed",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.String("id", session.ID()),
							log.String("status", session.Status()),
							log.Error(info.Error),
						)
					}
				}
			}
			t.OnSessionTransactionCommit = func(info trace.TableSessionTransactionCommitStartInfo) func(trace.TableSessionTransactionCommitDoneInfo) {
				session := info.Session
				tx := info.Tx
				l.Debug("committing",
					log.String("version", version),
					log.String("id", session.ID()),
					log.String("status", session.Status()),
					log.String("tx", tx.ID()),
				)
				start := time.Now()
				return func(info trace.TableSessionTransactionCommitDoneInfo) {
					if info.Error == nil {
						l.Debug("committed",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.String("id", session.ID()),
							log.String("status", session.Status()),
							log.String("tx", tx.ID()),
						)
					} else {
						l.Debug("commit failed",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.String("id", session.ID()),
							log.String("status", session.Status()),
							log.String("tx", tx.ID()),
							log.Error(info.Error),
						)
					}
				}
			}
			t.OnSessionTransactionRollback = func(info trace.TableSessionTransactionRollbackStartInfo) func(trace.TableSessionTransactionRollbackDoneInfo) {
				session := info.Session
				tx := info.Tx
				l.Debug("try to rollback",
					log.String("version", version),
					log.String("id", session.ID()),
					log.String("status", session.Status()),
					log.String("tx", tx.ID()),
				)
				start := time.Now()
				return func(info trace.TableSessionTransactionRollbackDoneInfo) {
					if info.Error == nil {
						l.Debug("rollback done",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.String("id", session.ID()),
							log.String("status", session.Status()),
							log.String("tx", tx.ID()),
						)
					} else {
						l.Error("rollback failed",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.String("id", session.ID()),
							log.String("status", session.Status()),
							log.String("tx", tx.ID()),
							log.Error(info.Error),
						)
					}
				}
			}
		}
	}
	if details&trace.TablePoolEvents != 0 {
		l := l.WithName("pool")
		if details&trace.TablePoolSessionLifeCycleEvents != 0 {
			l := l.WithName("session")
			t.OnPoolSessionNew = func(info trace.TablePoolSessionNewStartInfo) func(trace.TablePoolSessionNewDoneInfo) {
				l.Debug("try to create",
					log.String("version", version),
				)
				start := time.Now()
				return func(info trace.TablePoolSessionNewDoneInfo) {
					if info.Error == nil {
						session := info.Session
						l.Debug("created",
							log.String("version", version),
							log.String("id", session.ID()),
							log.String("status", session.Status()),
						)
					} else {
						l.Error("created",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.Error(info.Error),
						)
					}
				}
			}
			t.OnPoolSessionClose = func(info trace.TablePoolSessionCloseStartInfo) func(trace.TablePoolSessionCloseDoneInfo) {
				session := info.Session
				l.Debug("closing",
					log.String("version", version),
					log.String("id", session.ID()),
					log.String("status", session.Status()),
				)
				start := time.Now()
				return func(info trace.TablePoolSessionCloseDoneInfo) {
					l.Debug("closed",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.String("id", session.ID()),
						log.String("status", session.Status()),
					)
				}
			}
			t.OnPoolStateChange = func(info trace.TablePoolStateChangeInfo) {
				l.Info("change",
					log.String("version", version),
					log.Int("size", info.Size),
					log.String("event", info.Event),
				)
			}
		}
		if details&trace.TablePoolAPIEvents != 0 {
			t.OnPoolPut = func(info trace.TablePoolPutStartInfo) func(trace.TablePoolPutDoneInfo) {
				session := info.Session
				l.Debug("putting",
					log.String("version", version),
					log.String("id", session.ID()),
					log.String("status", session.Status()),
				)
				start := time.Now()
				return func(info trace.TablePoolPutDoneInfo) {
					if info.Error == nil {
						l.Debug("put",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.String("id", session.ID()),
							log.String("status", session.Status()),
						)
					} else {
						l.Error("put failed",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.String("id", session.ID()),
							log.String("status", session.Status()),
							log.Error(info.Error),
						)
					}
				}
			}
			t.OnPoolGet = func(info trace.TablePoolGetStartInfo) func(trace.TablePoolGetDoneInfo) {
				l.Debug("getting",
					log.String("version", version),
				)
				start := time.Now()
				return func(info trace.TablePoolGetDoneInfo) {
					if info.Error == nil {
						session := info.Session
						l.Debug("got",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.String("id", session.ID()),
							log.String("status", session.Status()),
							log.Int("attempts", info.Attempts),
						)
					} else {
						l.Warn("get failed",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.Int("attempts", info.Attempts),
							log.Error(info.Error),
						)
					}
				}
			}
			t.OnPoolWait = func(info trace.TablePoolWaitStartInfo) func(trace.TablePoolWaitDoneInfo) {
				l.Debug("waiting",
					log.String("version", version),
				)
				start := time.Now()
				return func(info trace.TablePoolWaitDoneInfo) {
					if info.Error == nil {
						if info.Session == nil {
							l.Debug(`wait done without any significant result`,
								log.String("version", version),
								log.Duration("latency", time.Since(start)),
							)
						} else {
							l.Debug(`wait done`,
								log.String("version", version),
								log.Duration("latency", time.Since(start)),
								log.String("id", info.Session.ID()),
								log.String("status", info.Session.Status()),
							)
						}
					} else {
						if info.Session == nil {
							l.Debug(`wait failed without any significant result`,
								log.String("version", version),
								log.Duration("latency", time.Since(start)),
								log.Error(info.Error),
							)
						} else {
							l.Debug(`wait failed`,
								log.String("version", version),
								log.Duration("latency", time.Since(start)),
								log.String("id", info.Session.ID()),
								log.String("status", info.Session.Status()),
								log.Error(info.Error),
							)
						}
					}
				}
			}
		}
	}
	return t
}
