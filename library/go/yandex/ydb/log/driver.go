package log

import (
	"time"

	ydbLog "github.com/ydb-platform/ydb-go-sdk/v3/log"
	"github.com/ydb-platform/ydb-go-sdk/v3/trace"

	"a.yandex-team.ru/library/go/core/log"
)

// Driver makes trace.Driver with zap logging
func Driver(l log.Logger, details trace.Details) (t trace.Driver) {
	l = l.WithName("ydb").WithName("driver")
	if details&trace.DriverNetEvents != 0 {
		l := l.WithName("net")
		t.OnNetRead = func(info trace.DriverNetReadStartInfo) func(trace.DriverNetReadDoneInfo) {
			address := info.Address
			l.Debug("try to read",
				log.String("version", version),
				log.String("address", address),
			)
			start := time.Now()
			return func(info trace.DriverNetReadDoneInfo) {
				if info.Error == nil {
					l.Debug("read",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.String("address", address),
						log.Int("received", info.Received),
					)
				} else {
					l.Warn("read failed",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.String("address", address),
						log.Int("received", info.Received),
						log.Error(info.Error),
					)
				}
			}
		}
		t.OnNetWrite = func(info trace.DriverNetWriteStartInfo) func(trace.DriverNetWriteDoneInfo) {
			address := info.Address
			l.Debug("try to write",
				log.String("version", version),
				log.String("address", address),
			)
			start := time.Now()
			return func(info trace.DriverNetWriteDoneInfo) {
				if info.Error == nil {
					l.Debug("wrote",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.String("address", address),
						log.Int("sent", info.Sent),
					)
				} else {
					l.Warn("write failed",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.String("address", address),
						log.Int("sent", info.Sent),
						log.Error(info.Error),
					)
				}
			}
		}
		t.OnNetDial = func(info trace.DriverNetDialStartInfo) func(trace.DriverNetDialDoneInfo) {
			address := info.Address
			l.Debug("try to dial",
				log.String("version", version),
				log.String("address", address),
			)
			start := time.Now()
			return func(info trace.DriverNetDialDoneInfo) {
				if info.Error == nil {
					l.Debug("dialed",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.String("address", address),
					)
				} else {
					l.Error("dial failed",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.String("address", address),
						log.Error(info.Error),
					)
				}
			}
		}
		t.OnNetClose = func(info trace.DriverNetCloseStartInfo) func(trace.DriverNetCloseDoneInfo) {
			address := info.Address
			l.Debug("try to close",
				log.String("version", version),
				log.String("address", address),
			)
			start := time.Now()
			return func(info trace.DriverNetCloseDoneInfo) {
				if info.Error == nil {
					l.Debug("closed",
						log.Duration("latency", time.Since(start)),
						log.String("version", version),
						log.String("address", address),
					)
				} else {
					l.Warn("close failed",
						log.Duration("latency", time.Since(start)),
						log.String("version", version),
						log.String("address", address),
						log.Error(info.Error),
					)
				}
			}
		}
	}
	if details&trace.DriverRepeaterEvents != 0 {
		l := l.WithName("repeater")
		t.OnRepeaterWakeUp = func(info trace.DriverRepeaterWakeUpStartInfo) func(trace.DriverRepeaterWakeUpDoneInfo) {
			name := info.Name
			event := info.Event
			l.Info("repeater wake up",
				log.String("version", version),
				log.String("name", name),
				log.String("event", event),
			)
			start := time.Now()
			return func(info trace.DriverRepeaterWakeUpDoneInfo) {
				if info.Error == nil {
					l.Info("repeater wake up done",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.String("name", name),
						log.String("event", event),
					)
				} else {
					l.Info("repeater wake up fail",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.String("name", name),
						log.String("event", event),
						log.Error(info.Error),
					)
				}
			}
		}
	}
	if details&trace.DriverConnEvents != 0 {
		l := l.WithName("conn")
		t.OnConnTake = func(info trace.DriverConnTakeStartInfo) func(trace.DriverConnTakeDoneInfo) {
			endpoint := info.Endpoint
			l.Debug("try to take conn",
				log.String("version", version),
				log.String("address", endpoint.Address()),
				log.Bool("localDC", endpoint.LocalDC()),
				log.String("location", endpoint.Location()),
				log.Time("lastUpdated", endpoint.LastUpdated()),
			)
			start := time.Now()
			return func(info trace.DriverConnTakeDoneInfo) {
				if info.Error == nil {
					l.Debug("conn took",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.String("address", endpoint.Address()),
						log.Bool("localDC", endpoint.LocalDC()),
						log.String("location", endpoint.Location()),
						log.Time("lastUpdated", endpoint.LastUpdated()),
					)
				} else {
					l.Warn("conn take failed",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.String("address", endpoint.Address()),
						log.Bool("localDC", endpoint.LocalDC()),
						log.String("location", endpoint.Location()),
						log.Time("lastUpdated", endpoint.LastUpdated()),
						log.Error(info.Error),
					)
				}
			}
		}
		t.OnConnStateChange = func(info trace.DriverConnStateChangeStartInfo) func(trace.DriverConnStateChangeDoneInfo) {
			endpoint := info.Endpoint
			l.Debug("conn state change",
				log.String("version", version),
				log.String("address", endpoint.Address()),
				log.Bool("localDC", endpoint.LocalDC()),
				log.String("location", endpoint.Location()),
				log.Time("lastUpdated", endpoint.LastUpdated()),
				log.String("state before", info.State.String()),
			)
			start := time.Now()
			return func(info trace.DriverConnStateChangeDoneInfo) {
				l.Debug("conn state changed",
					log.String("version", version),
					log.Duration("latency", time.Since(start)),
					log.String("address", endpoint.Address()),
					log.Bool("localDC", endpoint.LocalDC()),
					log.String("location", endpoint.Location()),
					log.Time("lastUpdated", endpoint.LastUpdated()),
					log.String("state after", info.State.String()),
				)
			}
		}
		t.OnConnInvoke = func(info trace.DriverConnInvokeStartInfo) func(trace.DriverConnInvokeDoneInfo) {
			endpoint := info.Endpoint
			method := string(info.Method)
			l.Debug("try to invoke",
				log.String("version", version),
				log.String("address", endpoint.Address()),
				log.Bool("localDC", endpoint.LocalDC()),
				log.String("location", endpoint.Location()),
				log.Time("lastUpdated", endpoint.LastUpdated()),
				log.String("method", method),
			)
			start := time.Now()
			return func(info trace.DriverConnInvokeDoneInfo) {
				if info.Error == nil {
					l.Debug("invoked",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.String("address", endpoint.Address()),
						log.Bool("localDC", endpoint.LocalDC()),
						log.String("location", endpoint.Location()),
						log.Time("lastUpdated", endpoint.LastUpdated()),
						log.String("method", method),
					)
				} else {
					l.Warn("invoke failed",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.String("address", endpoint.Address()),
						log.Bool("localDC", endpoint.LocalDC()),
						log.String("location", endpoint.Location()),
						log.Time("lastUpdated", endpoint.LastUpdated()),
						log.String("method", method),
						log.Error(info.Error),
					)
				}
			}
		}
		t.OnConnNewStream = func(info trace.DriverConnNewStreamStartInfo) func(trace.DriverConnNewStreamRecvInfo) func(trace.DriverConnNewStreamDoneInfo) {
			endpoint := info.Endpoint
			method := string(info.Method)
			l.Debug("try to streaming",
				log.String("version", version),
				log.String("address", endpoint.Address()),
				log.Bool("localDC", endpoint.LocalDC()),
				log.String("location", endpoint.Location()),
				log.Time("lastUpdated", endpoint.LastUpdated()),
				log.String("method", method),
			)
			start := time.Now()
			return func(info trace.DriverConnNewStreamRecvInfo) func(trace.DriverConnNewStreamDoneInfo) {
				if info.Error == nil {
					l.Debug("streaming intermediate receive",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.String("address", endpoint.Address()),
						log.Bool("localDC", endpoint.LocalDC()),
						log.String("location", endpoint.Location()),
						log.Time("lastUpdated", endpoint.LastUpdated()),
						log.String("method", method),
					)
				} else {
					l.Warn("streaming intermediate receive failed",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.String("address", endpoint.Address()),
						log.Bool("localDC", endpoint.LocalDC()),
						log.String("location", endpoint.Location()),
						log.Time("lastUpdated", endpoint.LastUpdated()),
						log.String("method", method),
						log.Error(info.Error),
					)
				}
				return func(info trace.DriverConnNewStreamDoneInfo) {
					if info.Error == nil {
						l.Debug("streaming finished",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.String("address", endpoint.Address()),
							log.Bool("localDC", endpoint.LocalDC()),
							log.String("location", endpoint.Location()),
							log.Time("lastUpdated", endpoint.LastUpdated()),
							log.String("method", method),
						)
					} else {
						l.Warn("streaming failed",
							log.String("version", version),
							log.Duration("latency", time.Since(start)),
							log.String("address", endpoint.Address()),
							log.Bool("localDC", endpoint.LocalDC()),
							log.String("location", endpoint.Location()),
							log.Time("lastUpdated", endpoint.LastUpdated()),
							log.String("method", method),
							log.Error(info.Error),
						)
					}
				}
			}
		}
	}
	if details&trace.DriverBalancerEvents != 0 {
		l := l.WithName("balancer")
		t.OnBalancerInit = func(info trace.DriverBalancerInitStartInfo) func(trace.DriverBalancerInitDoneInfo) {
			l.Debug("init start")
			start := time.Now()
			return func(info trace.DriverBalancerInitDoneInfo) {
				if info.Error == nil {
					l.Info("init done",
						log.Duration("latency", time.Since(start)),
					)
				} else {
					l.Info("init failed",
						log.Duration("latency", time.Since(start)),
						log.Error(info.Error),
					)
				}
			}
		}
		t.OnBalancerClose = func(info trace.DriverBalancerCloseStartInfo) func(trace.DriverBalancerCloseDoneInfo) {
			l.Debug("close start")
			start := time.Now()
			return func(info trace.DriverBalancerCloseDoneInfo) {
				if info.Error == nil {
					l.Debug("close done",
						log.Duration("latency", time.Since(start)),
					)
				} else {
					l.Warn("close failed",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.Error(info.Error),
					)
				}
			}
		}
		t.OnBalancerChooseEndpoint = func(info trace.DriverBalancerChooseEndpointStartInfo) func(doneInfo trace.DriverBalancerChooseEndpointDoneInfo) {
			l.Debug("try to choose endpoint")
			start := time.Now()
			return func(info trace.DriverBalancerChooseEndpointDoneInfo) {
				if info.Error == nil {
					l.Debug("endpoint choose ok",
						log.Duration("latency", time.Since(start)),
						log.String("address", info.Endpoint.Address()),
						log.Bool("local", info.Endpoint.LocalDC()),
					)
				} else {
					l.Warn("endpoint choose failed",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.Error(info.Error),
					)
				}
			}
		}
		t.OnBalancerUpdate = func(info trace.DriverBalancerUpdateStartInfo) func(trace.DriverBalancerUpdateDoneInfo) {
			l.Debug("try to update balancer",
				log.Bool("needLocalDC", info.NeedLocalDC),
			)
			start := time.Now()
			return func(info trace.DriverBalancerUpdateDoneInfo) {
				if info.Error == nil {
					endpoints := make([]string, 0, len(info.Endpoints))
					for _, e := range info.Endpoints {
						endpoints = append(endpoints, e.String())
					}
					l.Debug("endpoint choose ok",
						log.Duration("latency", time.Since(start)),
						log.Strings("endpoints", endpoints),
						log.String("local", info.LocalDC),
					)
				} else {
					l.Warn("endpoint choose failed",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.Error(info.Error),
					)
				}
			}
		}
	}
	if details&trace.DriverCredentialsEvents != 0 {
		l := l.WithName("credentials")
		t.OnGetCredentials = func(info trace.DriverGetCredentialsStartInfo) func(trace.DriverGetCredentialsDoneInfo) {
			l.Debug("getting",
				log.String("version", version),
			)
			start := time.Now()
			return func(info trace.DriverGetCredentialsDoneInfo) {
				if info.Error == nil {
					l.Debug("got",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.String("token", ydbLog.Secret(info.Token)),
					)
				} else {
					l.Error("get failed",
						log.String("version", version),
						log.Duration("latency", time.Since(start)),
						log.Error(info.Error),
					)
				}
			}
		}
	}
	return t
}
