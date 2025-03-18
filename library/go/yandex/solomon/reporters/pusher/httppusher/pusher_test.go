package httppusher

import (
	"context"
	"io/ioutil"
	stdlog "log"
	"net/http"
	"net/http/httptest"
	"net/url"
	"strings"
	"testing"
	"time"

	"github.com/go-resty/resty/v2"
	"github.com/google/go-cmp/cmp"
	"github.com/google/go-cmp/cmp/cmpopts"
	"github.com/pkg/errors"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	uzap "go.uber.org/zap"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/yandex/solomon/reporters/pusher"
)

func TestNewPusher(t *testing.T) {
	testCases := []struct {
		name      string
		opts      []PusherOpt
		expect    *Pusher
		expectErr error
	}{
		{
			"no_opts",
			nil,
			nil,
			ErrEmptyProject,
		},
		{
			"empty_project",
			[]PusherOpt{
				SetService("ololo"),
				SetCluster("trololo"),
			},
			nil,
			ErrEmptyProject,
		},
		{
			"empty_service",
			[]PusherOpt{
				SetProject("ololo"),
				SetCluster("trololo"),
			},
			nil,
			ErrEmptyService,
		},
		{
			"empty_cluster",
			[]PusherOpt{
				SetService("ololo"),
				SetProject("trololo"),
			},
			nil,
			ErrEmptyCluster,
		},
		{
			"default_host",
			[]PusherOpt{
				SetService("ololo"),
				SetProject("trololo"),
				SetCluster("shimba"),
			},
			&Pusher{
				logger: nil,
				httpc: resty.New().
					SetBaseURL(pusher.HostProduction).
					SetHeader("Content-Type", "application/json").
					SetQueryParams(map[string]string{
						"service": "ololo",
						"project": "trololo",
						"cluster": "shimba",
					}),
			},
			nil,
		},
		{
			"custom_host",
			[]PusherOpt{
				SetService("ololo"),
				SetProject("trololo"),
				SetCluster("shimba"),
				WithHTTPHost("https://golomon.yandex-team.ru"),
			},
			&Pusher{
				logger: nil,
				httpc: resty.New().
					SetBaseURL("https://golomon.yandex-team.ru").
					SetHeader("Content-Type", "application/json").
					SetQueryParams(map[string]string{
						"service": "ololo",
						"project": "trololo",
						"cluster": "shimba",
					}),
			},
			nil,
		},
		{
			"custom_logger",
			[]PusherOpt{
				SetService("ololo"),
				SetProject("trololo"),
				SetCluster("shimba"),
				WithLogger(zap.Must(zap.JSONConfig(log.DebugLevel))),
			},
			&Pusher{
				logger: zap.Must(zap.JSONConfig(log.DebugLevel)),
				httpc: resty.New().
					SetBaseURL("https://solomon.yandex.net").
					SetHeader("Content-Type", "application/json").
					SetQueryParams(map[string]string{
						"service": "ololo",
						"project": "trololo",
						"cluster": "shimba",
					}),
			},
			nil,
		},
		{
			"with_oauth",
			[]PusherOpt{
				SetService("ololo"),
				SetProject("trololo"),
				SetCluster("shimba"),
				WithOAuthToken("token-shmoken"),
			},
			&Pusher{
				logger: nil,
				httpc: resty.New().
					SetBaseURL("https://solomon.yandex.net").
					SetHeader("Content-Type", "application/json").
					SetHeader("Authorization", "OAuth token-shmoken").
					SetQueryParams(map[string]string{
						"service": "ololo",
						"project": "trololo",
						"cluster": "shimba",
					}),
			},
			nil,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			p, err := NewPusher(tc.opts...)

			if tc.expectErr != nil {
				assert.Error(t, err, tc.expectErr.Error())
			} else {
				assert.NoError(t, err)
			}

			opts := cmp.Options{
				cmp.AllowUnexported(Pusher{}),
				cmpopts.IgnoreUnexported(stdlog.Logger{}, resty.Client{}, uzap.Logger{}),
				cmpopts.IgnoreFields(resty.Client{}, "JSONMarshal", "JSONUnmarshal", "XMLMarshal", "XMLUnmarshal"),
			}

			assert.True(t, cmp.Equal(tc.expect, p, opts...), cmp.Diff(tc.expect, p, opts...))
		})
	}
}

func TestNewPusherWithResty(t *testing.T) {
	testCases := []struct {
		name      string
		client    *resty.Client
		opts      []PusherOpt
		expect    *Pusher
		expectErr error
	}{
		{
			"no_opts",
			resty.New(),
			nil,
			nil,
			ErrEmptyProject,
		},
		{
			"empty_project",
			resty.New(),
			[]PusherOpt{
				SetService("ololo"),
				SetCluster("trololo"),
			},
			nil,
			ErrEmptyProject,
		},
		{
			"empty_service",
			resty.New(),
			[]PusherOpt{
				SetProject("ololo"),
				SetCluster("trololo"),
			},
			nil,
			ErrEmptyService,
		},
		{
			"empty_cluster",
			resty.New(),
			[]PusherOpt{
				SetService("ololo"),
				SetProject("trololo"),
			},
			nil,
			ErrEmptyCluster,
		},
		{
			"default_host",
			resty.New(),
			[]PusherOpt{
				SetService("ololo"),
				SetProject("trololo"),
				SetCluster("shimba"),
			},
			&Pusher{
				logger: nil,
				httpc: resty.New().
					SetBaseURL(pusher.HostProduction).
					SetHeader("Content-Type", "application/json").
					SetQueryParams(map[string]string{
						"service": "ololo",
						"project": "trololo",
						"cluster": "shimba",
					}),
			},
			nil,
		},
		{
			"custom_host",
			resty.New(),
			[]PusherOpt{
				SetService("ololo"),
				SetProject("trololo"),
				SetCluster("shimba"),
				WithHTTPHost("https://golomon.yandex-team.ru"),
			},
			&Pusher{
				logger: nil,
				httpc: resty.New().
					SetBaseURL("https://golomon.yandex-team.ru").
					SetHeader("Content-Type", "application/json").
					SetQueryParams(map[string]string{
						"service": "ololo",
						"project": "trololo",
						"cluster": "shimba",
					}),
			},
			nil,
		},
		{
			"custom_logger",
			resty.New(),
			[]PusherOpt{
				SetService("ololo"),
				SetProject("trololo"),
				SetCluster("shimba"),
				WithLogger(zap.Must(zap.JSONConfig(log.DebugLevel))),
			},
			&Pusher{
				logger: zap.Must(zap.JSONConfig(log.DebugLevel)),
				httpc: resty.New().
					SetBaseURL("https://solomon.yandex.net").
					SetHeader("Content-Type", "application/json").
					SetQueryParams(map[string]string{
						"service": "ololo",
						"project": "trololo",
						"cluster": "shimba",
					}),
			},
			nil,
		},
		{
			"with_oauth",
			resty.New(),
			[]PusherOpt{
				SetService("ololo"),
				SetProject("trololo"),
				SetCluster("shimba"),
				WithOAuthToken("token-shmoken"),
			},
			&Pusher{
				logger: nil,
				httpc: resty.New().
					SetBaseURL("https://solomon.yandex.net").
					SetHeader("Content-Type", "application/json").
					SetHeader("Authorization", "OAuth token-shmoken").
					SetQueryParams(map[string]string{
						"service": "ololo",
						"project": "trololo",
						"cluster": "shimba",
					}),
			},
			nil,
		},
		{
			"custom_resty",
			resty.New().SetHeader("X-HTTP-Client", "CustomResty"),
			[]PusherOpt{
				SetService("ololo"),
				SetProject("trololo"),
				SetCluster("shimba"),
			},
			&Pusher{
				logger: nil,
				httpc: resty.New().
					SetBaseURL("https://solomon.yandex.net").
					SetHeader("Content-Type", "application/json").
					SetHeader("X-HTTP-Client", "CustomResty").
					SetQueryParams(map[string]string{
						"service": "ololo",
						"project": "trololo",
						"cluster": "shimba",
					}),
			},
			nil,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			p, err := NewPusherWithResty(tc.client, tc.opts...)

			if tc.expectErr != nil {
				assert.Error(t, err, tc.expectErr.Error())
			} else {
				assert.NoError(t, err)
			}

			opts := cmp.Options{
				cmp.AllowUnexported(Pusher{}),
				cmpopts.IgnoreUnexported(stdlog.Logger{}, resty.Client{}, uzap.Logger{}),
				cmpopts.IgnoreFields(resty.Client{}, "JSONMarshal", "JSONUnmarshal", "XMLMarshal", "XMLUnmarshal"),
			}

			assert.True(t, cmp.Equal(tc.expect, p, opts...), cmp.Diff(tc.expect, p, opts...))
		})
	}
}

func TestPusher_Push(t *testing.T) {
	testCases := []struct {
		name      string
		bootstrap func() (*httptest.Server, *Pusher)
		ctx       func() (context.Context, context.CancelFunc)
		metrics   *solomon.Metrics
		expectErr error
	}{
		{
			"context_timeout",
			func() (*httptest.Server, *Pusher) {
				ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					time.Sleep(1 * time.Second)
				}))
				p, err := NewPusher(
					SetProject("ololo"),
					SetCluster("trololo"),
					SetService("shimba"),
					WithHTTPHost(ts.URL),
				)

				require.NoError(t, err)

				return ts, p
			},
			func() (context.Context, context.CancelFunc) {
				return context.WithTimeout(context.Background(), 100*time.Millisecond)
			},
			func() *solomon.Metrics {
				r := solomon.NewRegistry(solomon.NewRegistryOpts())
				sg := r.Gauge("mygauge")
				sg.Set(42)

				metrics, err := r.Gather()
				assert.NoError(t, err)

				return metrics
			}(),
			errors.New("context deadline exceeded"),
		},
		{
			"gateway_timeout",
			func() (*httptest.Server, *Pusher) {
				ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					w.WriteHeader(http.StatusGatewayTimeout)
				}))

				p, err := NewPusher(
					SetProject("ololo"),
					SetCluster("trololo"),
					SetService("shimba"),
					WithHTTPHost(ts.URL),
				)

				require.NoError(t, err)

				return ts, p
			},
			func() (context.Context, context.CancelFunc) {
				return context.WithCancel(context.Background())
			},
			func() *solomon.Metrics {
				r := solomon.NewRegistry(solomon.NewRegistryOpts())
				sg := r.Gauge("mygauge")
				sg.Set(42)

				metrics, err := r.Gather()
				assert.NoError(t, err)

				return metrics
			}(),
			ErrSendGatewayTimeout,
		},
		{
			"unsupported_status_code",
			func() (*httptest.Server, *Pusher) {
				ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					w.WriteHeader(http.StatusUnauthorized)
					_, _ = w.Write([]byte(`given OAuth token expired`))
				}))

				p, err := NewPusher(
					SetProject("ololo"),
					SetCluster("trololo"),
					SetService("shimba"),
					WithHTTPHost(ts.URL),
				)

				require.NoError(t, err)

				return ts, p
			},
			func() (context.Context, context.CancelFunc) {
				return context.WithCancel(context.Background())
			},
			func() *solomon.Metrics {
				r := solomon.NewRegistry(solomon.NewRegistryOpts())
				sg := r.Gauge("mygauge")
				sg.Set(42)

				metrics, err := r.Gather()
				assert.NoError(t, err)

				return metrics
			}(),
			errors.New("bad status code 401: given OAuth token expired"),
		},
		{
			"success",
			func() (*httptest.Server, *Pusher) {
				ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					expectedQueryArgs := url.Values{
						"project": []string{"ololo"},
						"cluster": []string{"trololo"},
						"service": []string{"shimba"},
					}
					expectedBody := []byte(`{"metrics":[{"type":"DGAUGE","labels":{"sensor":"MemoryHeap"},"value":42}]}`)

					assert.Equal(t, expectedQueryArgs, r.URL.Query())

					body, err := ioutil.ReadAll(r.Body)
					assert.NoError(t, err)
					assert.Equal(t, expectedBody, body)
				}))

				p, err := NewPusher(
					SetProject("ololo"),
					SetCluster("trololo"),
					SetService("shimba"),
					WithHTTPHost(ts.URL),
				)

				require.NoError(t, err)

				return ts, p
			},
			func() (context.Context, context.CancelFunc) {
				return context.WithCancel(context.Background())
			},
			func() *solomon.Metrics {
				r := solomon.NewRegistry(solomon.NewRegistryOpts())
				sg := r.Gauge("MemoryHeap")
				sg.Set(42)

				metrics, err := r.Gather()
				assert.NoError(t, err)

				return metrics
			}(),
			nil,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			srv, p := tc.bootstrap()
			defer srv.Close()

			ctx, cancel := tc.ctx()
			defer cancel()

			err := p.Push(ctx, tc.metrics)

			if tc.expectErr == nil {
				assert.NoError(t, err)
			} else {
				equals := err != nil && (err.Error() == tc.expectErr.Error() || strings.Contains(err.Error(), tc.expectErr.Error()))
				assert.True(t, equals, "got error: '%s'", err)
			}
		})
	}
}
