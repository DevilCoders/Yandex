package httpbb_test

import (
	"context"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"go.uber.org/zap/zaptest/observer"

	"a.yandex-team.ru/library/go/core/log"
	aZap "a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/yandex/blackbox"
	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb"
)

func TestClient_Leak(t *testing.T) {
	type Bootstrap struct {
		srv    *httptest.Server
		client *httpbb.Client
		logs   *observer.ObservedLogs
	}

	type TestCase struct {
		bootstrap func() Bootstrap
	}

	newLogger := func() (*aZap.Logger, *observer.ObservedLogs) {
		logger, err := aZap.New(aZap.ConsoleConfig(log.DebugLevel))
		require.NoError(t, err)

		core, logs := observer.New(aZap.ZapifyLevel(log.DebugLevel))
		logger.L = logger.L.WithOptions(zap.WrapCore(func(_ zapcore.Core) zapcore.Core {
			return core
		}))

		return logger, logs
	}
	cases := map[string]TestCase{
		"no_connection": {
			bootstrap: func() Bootstrap {
				var srv *httptest.Server
				srv = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					srv.CloseClientConnections()
				}))
				srv.Close()

				l, logs := newLogger()
				c, err := httpbb.NewClient(
					httpbb.Environment{BlackboxHost: srv.URL},
					httpbb.WithLogger(l),
				)
				require.NoError(t, err)

				return Bootstrap{
					srv:    srv,
					client: c,
					logs:   logs,
				}
			},
		},
		"net_error": {
			bootstrap: func() Bootstrap {
				var srv *httptest.Server
				srv = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					srv.CloseClientConnections()
				}))

				l, logs := newLogger()
				c, err := httpbb.NewClient(
					httpbb.Environment{BlackboxHost: srv.URL},
					httpbb.WithLogger(l),
				)
				require.NoError(t, err)

				return Bootstrap{
					srv:    srv,
					client: c,
					logs:   logs,
				}
			},
		},
		"bad_status": {
			bootstrap: func() Bootstrap {
				srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					w.WriteHeader(http.StatusBadRequest)
				}))

				l, logs := newLogger()
				c, err := httpbb.NewClient(
					httpbb.Environment{BlackboxHost: srv.URL},
					httpbb.WithLogger(l),
				)
				require.NoError(t, err)

				return Bootstrap{
					srv:    srv,
					client: c,
					logs:   logs,
				}
			},
		},
		"bad_body": {
			bootstrap: func() Bootstrap {
				srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					_, _ = w.Write([]byte(`not a json response`))
				}))

				l, logs := newLogger()
				c, err := httpbb.NewClient(
					httpbb.Environment{BlackboxHost: srv.URL},
					httpbb.WithLogger(l),
				)
				require.NoError(t, err)

				return Bootstrap{
					srv:    srv,
					client: c,
					logs:   logs,
				}
			},
		},
	}

	logsNotContains := func(t *testing.T, logs []observer.LoggedEntry, val string) {
		for _, entry := range logs {
			require.NotContains(t, entry.Message, val)
			for _, field := range entry.Context {
				require.NotContains(t, field.Key, val)
				require.NotContains(t, field.String, val)
			}
		}
	}

	tester := func(name string, tc TestCase) {
		t.Run(name, func(t *testing.T) {
			t.Parallel()

			bootstrap := tc.bootstrap()
			defer bootstrap.srv.Close()

			t.Run("sessid", func(t *testing.T) {
				sessID := "sessid-val"
				_, err := bootstrap.client.SessionID(context.Background(), blackbox.SessionIDRequest{
					SessionID: sessID,
					UserIP:    "1.1.1.1",
					Host:      "test",
				})
				require.Error(t, err)
				require.NotContains(t, err.Error(), sessID)

				logs := bootstrap.logs.TakeAll()
				assert.NotEmpty(t, logs)
				logsNotContains(t, logs, sessID)
			})

			t.Run("multisessid", func(t *testing.T) {
				sessID := "multisessid-val"
				_, err := bootstrap.client.MultiSessionID(context.Background(), blackbox.MultiSessionIDRequest{
					SessionID: sessID,
					UserIP:    "1.1.1.1",
					Host:      "test",
				})
				require.Error(t, err)
				require.NotContains(t, err.Error(), sessID)

				logs := bootstrap.logs.TakeAll()
				assert.NotEmpty(t, logs)
				logsNotContains(t, logs, sessID)
			})

			t.Run("oauth", func(t *testing.T) {
				oauthToken := "oauth-token"
				_, err := bootstrap.client.OAuth(context.Background(), blackbox.OAuthRequest{
					OAuthToken: oauthToken,
					UserIP:     "1.1.1.1",
				})
				require.Error(t, err)
				require.NotContains(t, err.Error(), oauthToken)

				logs := bootstrap.logs.TakeAll()
				assert.NotEmpty(t, logs)
				logsNotContains(t, logs, oauthToken)
			})

			t.Run("user_ticket", func(t *testing.T) {
				ticket := "user-ticket"
				_, err := bootstrap.client.UserTicket(context.Background(), blackbox.UserTicketRequest{
					UserTicket: ticket,
				})
				require.Error(t, err)
				require.NotContains(t, err.Error(), ticket)

				logs := bootstrap.logs.TakeAll()
				assert.NotEmpty(t, logs)
				logsNotContains(t, logs, ticket)
			})
		})
	}

	for name, tc := range cases {
		tester(name, tc)
	}
}
