package httputil

import (
	"context"
	"fmt"
	"net/http"
	"net/url"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	sentrymocks "a.yandex-team.ru/cloud/mdb/internal/sentry/mocks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func Test_gatherSentryTags(t *testing.T) {
	t.Run("it doesn't fail on nil", func(t *testing.T) {
		require.Nil(t, gatherSentryTags(nil))
	})

	t.Run("headers", func(t *testing.T) {
		require.Equal(t,
			map[string]string{
				"user_agent":        "unit tests",
				"method":            "GET",
				"request_uri":       "/ping",
				"x_forwarded_for":   "1.1.1.1",
				"remote_addr":       "127.0.0.1",
				"x_forwarded_agent": "curl",
			},
			gatherSentryTags(
				&http.Request{
					Method: http.MethodGet,
					URL: &url.URL{
						Scheme: "https",
						Host:   "localhost",
						Path:   "/ping",
					},
					RemoteAddr: "127.0.0.1",
					Header: map[string][]string{
						"User-Agent":        {"unit tests"},
						"X-Forwarded-For":   {"1.1.1.1"},
						"X-Forwarded-Agent": {"curl"},
					},
				},
			),
		)
	})
}

func TestReportErrorToSentry(t *testing.T) {
	for _, notReportedErr := range []error{nil, context.DeadlineExceeded} {
		t.Run(fmt.Sprintf("%v shouldn't be reported", notReportedErr), func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			c := sentrymocks.NewMockClient(ctrl)
			sentry.SetGlobalClient(c)

			ReportErrorToSentry(notReportedErr, &http.Request{})
		})
	}
	t.Run("unknown errors should be reported", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		c := sentrymocks.NewMockClient(ctrl)
		sentry.SetGlobalClient(c)
		unknownError := xerrors.New("something bad happens")
		c.EXPECT().CaptureError(gomock.Any(), unknownError, gomock.Any())

		ReportErrorToSentry(unknownError, &http.Request{})
	})
}
