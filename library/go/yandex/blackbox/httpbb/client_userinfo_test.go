package httpbb_test

import (
	"context"
	"encoding/json"
	"os"
	"path/filepath"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yandex/blackbox"
	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb"
)

func TestClient_UserInfo(t *testing.T) {
	type TestCase struct {
		BlackboxHTTPRsp
		Request          blackbox.UserInfoRequest   `json:"REQUEST"`
		ExpectedResponse *blackbox.UserInfoResponse `json:"EXPECTED_RESPONSE"`
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

			response, err := bbClient.UserInfo(context.Background(), tc.Request)
			require.NoError(t, err)

			require.EqualValuesf(t, tc.ExpectedResponse, response, "request: %+v", tc.Request)
		})
	}

	cases, err := listCases("userinfo")
	require.NoError(t, err)
	require.NotEmpty(t, cases)

	for _, c := range cases {
		tester(c)
	}
}

func TestClient_UserInfoErrors(t *testing.T) {
	cases := map[string]struct {
		request blackbox.UserInfoRequest
		err     error
	}{
		"empty": {
			request: blackbox.UserInfoRequest{},
			err:     blackbox.ErrRequestNoUIDOrLogin,
		},
		"no_userip": {
			request: blackbox.UserInfoRequest{
				Login: "test",
			},
			err: blackbox.ErrRequestNoUserIP,
		},
		"no_email_to_test": {
			request: blackbox.UserInfoRequest{
				Login:  "test",
				UserIP: "1.1.1.1",
				Emails: blackbox.EmailTestOne,
			},
			err: blackbox.ErrRequestNoEmailToTest,
		},
	}

	for name, tc := range cases {
		t.Run(name, func(t *testing.T) {
			bbClient, err := httpbb.NewClient(httpbb.Environment{
				BlackboxHost: "not-used",
			})
			require.NoError(t, err)

			_, err = bbClient.UserInfo(context.Background(), tc.request)
			require.IsType(t, tc.err, err)
			require.EqualError(t, err, tc.err.Error())
		})
	}
}
