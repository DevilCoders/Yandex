package ready

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/mdb/internal/ready/mocks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestWait_Success(t *testing.T) {
	ctx := context.Background()
	ctrl := gomock.NewController(t)
	c := mocks.NewMockChecker(ctrl)

	c.EXPECT().IsReady(gomock.Any()).Return(nil)
	err := Wait(ctx, c, &DefaultErrorTester{}, time.Millisecond)
	assert.NoError(t, err)
}

func TestWait_DefaultPermanentFailure(t *testing.T) {
	ctx := context.Background()
	ctrl := gomock.NewController(t)
	c := mocks.NewMockChecker(ctrl)

	failure := xerrors.New("failure")
	c.EXPECT().IsReady(gomock.Any()).Return(failure)
	err := WaitWithTimeout(ctx, time.Second, c, &DefaultErrorTester{FailOnError: true}, time.Millisecond)
	assert.Error(t, err)
	assert.True(t, xerrors.Is(err, failure))
}

func TestWait_TimeoutFailure(t *testing.T) {
	ctx := context.Background()
	ctrl := gomock.NewController(t)
	c := mocks.NewMockChecker(ctrl)

	firstFailure := xerrors.New("first")
	secondFailure := xerrors.New("second")

	// First error happens one
	firstCall := c.EXPECT().IsReady(gomock.Any()).Return(firstFailure)
	// Then second error happens until context timeouts
	c.EXPECT().IsReady(gomock.Any()).Return(secondFailure).AnyTimes().After(firstCall)
	err := WaitWithTimeout(ctx, time.Second, c, &DefaultErrorTester{}, time.Millisecond)
	assert.Error(t, err)
	assert.True(t, xerrors.Is(err, secondFailure))
}

type customErrorTester struct {
	Permanent error
}

var _ ErrorTester = &customErrorTester{}

func (et *customErrorTester) IsPermanentError(_ context.Context, err error) bool {
	if et.Permanent == nil {
		return false
	}

	return xerrors.Is(err, et.Permanent)
}

func TestWait_CustomErrorTester(t *testing.T) {
	ctx := context.Background()
	ctrl := gomock.NewController(t)
	c := mocks.NewMockChecker(ctrl)

	firstFailure := xerrors.New("first")
	secondFailure := xerrors.New("second")

	et := &customErrorTester{Permanent: secondFailure}

	// First error happens one
	firstCall := c.EXPECT().IsReady(gomock.Any()).Return(firstFailure)
	// Then second error happens and it should be a permanent error
	c.EXPECT().IsReady(gomock.Any()).Return(secondFailure).After(firstCall)
	err := Wait(ctx, c, et, time.Millisecond)
	assert.Error(t, err)
	assert.True(t, xerrors.Is(err, secondFailure))
}
