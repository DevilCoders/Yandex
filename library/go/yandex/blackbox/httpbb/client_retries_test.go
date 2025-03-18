package httpbb_test

import (
	"context"
	"fmt"
	"net/http"
	"net/http/httptest"
	"sync/atomic"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yandex/blackbox"
	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb"
)

func TestClient_Retries(t *testing.T) {
	type Bootstrap struct {
		srv    *httptest.Server
		client *httpbb.Client
		calls  *int32
	}

	type TestCase struct {
		expectedRetries int32
		bootstrap       func() Bootstrap
	}

	const bbRetries = 2
	cases := map[string]TestCase{
		"net_error": {
			expectedRetries: bbRetries,
			bootstrap: func() Bootstrap {
				calls := new(int32)
				var srv *httptest.Server
				srv = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					atomic.AddInt32(calls, 1)
					srv.CloseClientConnections()
				}))

				c, err := httpbb.NewClient(
					httpbb.Environment{BlackboxHost: srv.URL},
					httpbb.WithRetries(bbRetries),
				)
				require.NoError(t, err)

				return Bootstrap{
					srv:    srv,
					client: c,
					calls:  calls,
				}
			},
		},
		"net_error_override": {
			expectedRetries: 1,
			bootstrap: func() Bootstrap {
				calls := new(int32)
				var srv *httptest.Server
				srv = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					atomic.AddInt32(calls, 1)
					srv.CloseClientConnections()
				}))

				c, err := httpbb.NewClient(
					httpbb.Environment{BlackboxHost: srv.URL},
					httpbb.WithRetries(1),
				)
				require.NoError(t, err)

				return Bootstrap{
					srv:    srv,
					client: c,
					calls:  calls,
				}
			},
		},
		"http_status_error": {
			expectedRetries: bbRetries,
			bootstrap: func() Bootstrap {
				calls := new(int32)
				srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					atomic.AddInt32(calls, 1)
					w.WriteHeader(http.StatusBadRequest)
				}))

				c, err := httpbb.NewClient(
					httpbb.Environment{BlackboxHost: srv.URL},
					httpbb.WithRetries(bbRetries),
				)
				require.NoError(t, err)

				return Bootstrap{
					srv:    srv,
					client: c,
					calls:  calls,
				}
			},
		},
		"parse_error": {
			expectedRetries: bbRetries,
			bootstrap: func() Bootstrap {
				calls := new(int32)
				srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					atomic.AddInt32(calls, 1)
					_, _ = w.Write([]byte(`not a json response`))
				}))

				c, err := httpbb.NewClient(
					httpbb.Environment{BlackboxHost: srv.URL},
					httpbb.WithRetries(bbRetries),
				)
				require.NoError(t, err)

				return Bootstrap{
					srv:    srv,
					client: c,
					calls:  calls,
				}
			},
		},
		"blackbox_error_DB_EXCEPTION": {
			expectedRetries: bbRetries,
			bootstrap: func() Bootstrap {
				calls := new(int32)
				srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					atomic.AddInt32(calls, 1)
					_, _ = w.Write([]byte(`{"exception":{"value":"DB_EXCEPTION","id":10},"error":"BlackBox error: something terrible. too"}`))
				}))

				c, err := httpbb.NewClient(
					httpbb.Environment{BlackboxHost: srv.URL},
					httpbb.WithRetries(bbRetries),
				)
				require.NoError(t, err)

				return Bootstrap{
					srv:    srv,
					client: c,
					calls:  calls,
				}
			},
		},
		"blackbox_error_INVALID_PARAMS": {
			expectedRetries: 1,
			bootstrap: func() Bootstrap {
				calls := new(int32)
				srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					atomic.AddInt32(calls, 1)
					_, _ = w.Write([]byte(`{"exception":{"value":"INVALID_PARAMS","id":2},"error":"BlackBox error: Missing userip argument"}`))
				}))

				c, err := httpbb.NewClient(
					httpbb.Environment{BlackboxHost: srv.URL},
					httpbb.WithRetries(bbRetries),
				)
				require.NoError(t, err)

				return Bootstrap{
					srv:    srv,
					client: c,
					calls:  calls,
				}
			},
		},
		"blackbox_error_UNKNOWN": {
			expectedRetries: 1,
			bootstrap: func() Bootstrap {
				calls := new(int32)
				srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					atomic.AddInt32(calls, 1)
					fmt.Println("CALL!", r.URL)
					_, _ = w.Write([]byte(`{"exception":{"value":"UNKNOWN","id":1},"error":"BlackBox error: Generic error"}`))
				}))

				c, err := httpbb.NewClient(
					httpbb.Environment{BlackboxHost: srv.URL},
					httpbb.WithRetries(bbRetries),
				)
				require.NoError(t, err)

				return Bootstrap{
					srv:    srv,
					client: c,
					calls:  calls,
				}
			},
		},
		"status_error": {
			expectedRetries: 1,
			bootstrap: func() Bootstrap {
				calls := new(int32)
				srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					atomic.AddInt32(calls, 1)
					_, _ = w.Write([]byte(`{"error": "hostname specified doesn't belong to cookie domain","status": {"id": 5,"value": "INVALID"}}`))
				}))

				c, err := httpbb.NewClient(
					httpbb.Environment{BlackboxHost: srv.URL},
					httpbb.WithRetries(bbRetries),
				)
				require.NoError(t, err)

				return Bootstrap{
					srv:    srv,
					client: c,
					calls:  calls,
				}
			},
		},
	}

	tester := func(name string, tc TestCase) {
		t.Run(name, func(t *testing.T) {
			t.Parallel()

			bootstrap := tc.bootstrap()
			defer bootstrap.srv.Close()

			t.Run("sessid", func(t *testing.T) {
				atomic.StoreInt32(bootstrap.calls, 0)
				_, err := bootstrap.client.SessionID(context.Background(), blackbox.SessionIDRequest{
					SessionID: "sessid-val",
					UserIP:    "1.1.1.1",
					Host:      "test",
				})
				require.Error(t, err)
				require.Equal(t, tc.expectedRetries, atomic.LoadInt32(bootstrap.calls))
			})

			t.Run("multisessid", func(t *testing.T) {
				atomic.StoreInt32(bootstrap.calls, 0)
				_, err := bootstrap.client.MultiSessionID(context.Background(), blackbox.MultiSessionIDRequest{
					SessionID: "multisessid-val",
					UserIP:    "1.1.1.1",
					Host:      "test",
				})
				require.Error(t, err)
				require.Equal(t, tc.expectedRetries, atomic.LoadInt32(bootstrap.calls))
			})

			t.Run("oauth", func(t *testing.T) {
				atomic.StoreInt32(bootstrap.calls, 0)
				_, err := bootstrap.client.OAuth(context.Background(), blackbox.OAuthRequest{
					OAuthToken: "oauth-token",
					UserIP:     "1.1.1.1",
				})
				require.Error(t, err)
				require.Equal(t, tc.expectedRetries, atomic.LoadInt32(bootstrap.calls))
			})
		})
	}

	for name, tc := range cases {
		tester(name, tc)
	}
}
