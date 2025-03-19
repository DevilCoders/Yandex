package sentry_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"

	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	sentrymocks "a.yandex-team.ru/cloud/mdb/internal/sentry/mocks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestCapturePanicAndWait(t *testing.T) {
	t.Run("For an error", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		c := sentrymocks.NewMockClient(ctrl)
		sentry.SetGlobalClient(c)

		sampleErr := xerrors.New("test error")
		tags := map[string]string{"foo": "bar"}
		c.EXPECT().CaptureErrorAndWait(gomock.Any(), sampleErr, tags)

		sentry.CapturePanicAndWait(context.Background(), sampleErr, tags)
	})

	t.Run("For a string", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		c := sentrymocks.NewMockClient(ctrl)
		sentry.SetGlobalClient(c)

		tags := map[string]string{"foo": "bar"}
		c.EXPECT().CaptureErrorAndWait(gomock.Any(), gomock.Any(), tags)

		sentry.CapturePanicAndWait(context.Background(), "foo", tags)
	})
}
