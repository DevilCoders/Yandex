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

func TestClient_UserTicket(t *testing.T) {
	type TestCase struct {
		BlackboxHTTPRsp
		Request          blackbox.UserTicketRequest   `json:"REQUEST"`
		ExpectedResponse *blackbox.UserTicketResponse `json:"EXPECTED_RESPONSE"`
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

			response, err := bbClient.UserTicket(context.Background(), tc.Request)
			require.NoError(t, err)

			require.EqualValuesf(t, tc.ExpectedResponse, response, "request: %+v", tc.Request)
		})
	}

	cases, err := listCases("userticket")
	require.NoError(t, err)
	require.NotEmpty(t, cases)

	for _, c := range cases {
		tester(c)
	}
}

func TestClient_UserTicketErrors(t *testing.T) {
	cases := map[string]struct {
		request blackbox.UserTicketRequest
		err     error
	}{
		"empty": {
			request: blackbox.UserTicketRequest{},
			err:     blackbox.ErrRequestNoUserTicket,
		},
		"uids_and_multi_conflict_test": {
			request: blackbox.UserTicketRequest{
				UserTicket: "test",
				UIDs:       []blackbox.ID{123},
				Multi:      true,
			},
			err: blackbox.ErrRequestUIDsandMultiConflict,
		},
		"no_email_to_test": {
			request: blackbox.UserTicketRequest{
				Emails:     blackbox.EmailTestOne,
				UserTicket: "test",
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

			_, err = bbClient.UserTicket(context.Background(), tc.request)
			require.IsType(t, tc.err, err)
			require.EqualError(t, err, tc.err.Error())
		})
	}
}
