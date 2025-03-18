package httpbb_test

import (
	"context"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yandex/blackbox"
	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb"
)

func TestClient_OAuth(t *testing.T) {
	type TestCase struct {
		BlackboxHTTPRsp
		Request          blackbox.OAuthRequest   `json:"REQUEST"`
		ExpectedResponse *blackbox.OAuthResponse `json:"EXPECTED_RESPONSE"`
	}

	tester := func(casePath string) {
		t.Run(filepath.Base(casePath), func(t *testing.T) {
			t.Parallel()

			rawCase, err := os.Open(casePath)
			require.NoError(t, err)
			defer func() { _ = rawCase.Close() }()

			dec := json.NewDecoder(rawCase)
			dec.DisallowUnknownFields()

			var tc TestCase
			err = dec.Decode(&tc)
			require.NoError(t, err)

			srv := NewBlackBoxSrv(t, tc.BlackboxHTTPRsp)
			defer srv.Close()

			bbClient, err := httpbb.NewClient(httpbb.Environment{
				BlackboxHost: srv.URL,
			})
			require.NoError(t, err)

			response, err := bbClient.OAuth(context.Background(), tc.Request)
			require.NoError(t, err)

			require.EqualValuesf(t, tc.ExpectedResponse, response, "request: %+v", tc.Request)
		})
	}

	cases, err := listCases("oauth")
	require.NoError(t, err)
	require.NotEmpty(t, cases)

	for _, c := range cases {
		tester(c)
	}
}

func TestClient_OAuthErrors(t *testing.T) {
	cases := map[string]struct {
		request blackbox.OAuthRequest
		err     error
	}{
		"empty": {
			request: blackbox.OAuthRequest{},
			err:     blackbox.ErrRequestNoOAuthToken,
		},
		"no_userip": {
			request: blackbox.OAuthRequest{
				OAuthToken: "test",
			},
			err: blackbox.ErrRequestNoUserIP,
		},
		"no_email_to_test": {
			request: blackbox.OAuthRequest{
				OAuthToken: "test",
				UserIP:     "1.1.1.1",
				Emails:     blackbox.EmailTestOne,
			},
			err: blackbox.ErrRequestNoEmailToTest,
		},
		"no_tvm": {
			request: blackbox.OAuthRequest{
				OAuthToken:    "test",
				UserIP:        "1.1.1.1",
				Emails:        blackbox.EmailTestOne,
				AddrToTest:    "test@yandex.ru",
				GetUserTicket: true,
			},
			err: blackbox.ErrRequestTvmNotAvailable,
		},
	}

	for name, tc := range cases {
		t.Run(name, func(t *testing.T) {
			bbClient, err := httpbb.NewClient(httpbb.Environment{
				BlackboxHost: "not-used",
			})
			require.NoError(t, err)

			_, err = bbClient.OAuth(context.Background(), tc.request)
			require.IsType(t, tc.err, err)
			require.EqualError(t, err, tc.err.Error())
		})
	}
}

func TestClient_OAuthStatuses(t *testing.T) {
	type TestCase struct {
		err       error
		bootstrap func() *httptest.Server
	}

	cases := map[string]TestCase{
		"invalid": {
			err: &blackbox.StatusError{
				Status:  blackbox.StatusInvalid,
				Message: "hostname specified doesn't belong to cookie domain",
			},
			bootstrap: func() *httptest.Server {
				return httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					_, _ = w.Write([]byte(`{"error": "hostname specified doesn't belong to cookie domain","status": {"id": 5,"value": "INVALID"}}`))
				}))
			},
		},
		"disabled": {
			err: &blackbox.UnauthorizedError{
				StatusError: blackbox.StatusError{
					Status:  blackbox.StatusDisabled,
					Message: "account was disabled by support",
				},
			},
			bootstrap: func() *httptest.Server {
				return httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					_, _ = w.Write([]byte(`{"error": "account was disabled by support","status": {"id": 4,"value": "DISABLED"}}`))
				}))
			},
		},
		// NOAUTH status not possible for oauth-token
		"noauth": {
			err: &blackbox.StatusError{
				Status:  blackbox.StatusNoAuth,
				Message: "no auth",
			},
			bootstrap: func() *httptest.Server {
				return httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					_, _ = w.Write([]byte(`{"error": "no auth","status": {"id": 3,"value": "NOAUTH"}}`))
				}))
			},
		},
		// EXPIRED status not possible for oauth-token
		"expired": {
			err: &blackbox.StatusError{
				Status:  blackbox.StatusExpired,
				Message: "expired cookie",
			},
			bootstrap: func() *httptest.Server {
				return httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					_, _ = w.Write([]byte(`{"error": "expired cookie","status": {"id": 2,"value": "EXPIRED"}}`))
				}))
			},
		},
	}

	tester := func(name string, tc TestCase) {
		t.Run(name, func(t *testing.T) {
			srv := tc.bootstrap()
			defer srv.Close()

			bbClient, err := httpbb.NewClient(
				httpbb.Environment{BlackboxHost: srv.URL},
				httpbb.WithRetries(1),
			)
			require.NoError(t, err)

			_, err = bbClient.OAuth(context.Background(), blackbox.OAuthRequest{
				OAuthToken: "test-token",
				UserIP:     "1.1.1.1",
			})
			require.IsType(t, tc.err, err)
			require.EqualValues(t, tc.err, err)
		})
	}

	for name, tc := range cases {
		tester(name, tc)
	}
}
