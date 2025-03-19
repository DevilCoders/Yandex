package logbroker

import (
	"context"
	"errors"
	"fmt"
	"io"
	"sync"
	"testing"
	"time"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
)

type mockServiceSuite struct {
	suite.Suite
	srvHelper

	mock lbReaderMock
}

func TestMockService(t *testing.T) {
	suite.Run(t, new(mockServiceSuite))
}

func (suite *mockServiceSuite) SetupTest() {
	suite.mock = lbReaderMock{}

	suite.setupService(&suite.Suite, "")
	suite.srv.queueOverride = func(persqueue.ReaderOptions) persqueue.Reader {
		return &suite.mock
	}
}

func (suite *mockServiceSuite) TestRun() {
	suite.mock.makeEventsMocks(true)

	suite.mock.On("Err").Return(errFatal.Wrap(errTestDone))

	err := suite.srv.Run()
	suite.Require().Error(err)
	suite.Require().ErrorIs(err, errTestDone)
	suite.mock.AssertExpectations(suite.T())
}

func (suite *mockServiceSuite) TestConsumeError() {
	suite.mock.makeEventsMocks(true)

	suspendCalled := false
	suite.srv.suspendTimeOverride = func() time.Duration {
		suspendCalled = true
		return time.Microsecond
	}

	suite.mock.On("Err").Return(errors.New("loop error")).Once()
	suite.mock.On("Err").Return(errFatal.Wrap(errTestDone)).Once()

	err := suite.srv.Run()
	suite.Require().Error(err)
	suite.Require().ErrorIs(err, errTestDone)

	suite.True(suspendCalled)
	suite.mock.AssertExpectations(suite.T())
}

func (suite *mockServiceSuite) TestConsumeReconnect() {
	suite.mock.makeEventsMocks(true)

	suspendCalled := false
	suite.srv.suspendTimeOverride = func() time.Duration {
		suspendCalled = true
		return time.Microsecond
	}

	suite.mock.On("Err").Return(nil).Once()
	suite.mock.On("Err").Return(errFatal.Wrap(errTestDone)).Once()

	err := suite.srv.Run()
	suite.Require().Error(err)
	suite.Require().ErrorIs(err, errTestDone)

	suite.False(suspendCalled)
	suite.mock.AssertExpectations(suite.T())
}

func (suite *mockServiceSuite) TestShutdown() {
	ev := suite.mock.makeEventsMocks(false)

	suite.mock.On("Shutdown").Return().Run(func(mock.Arguments) {
		close(ev)
	})
	suite.mock.On("Err").Return(errFatal.Wrap(errTestDone)).Once()

	wg := sync.WaitGroup{}
	wg.Add(1)

	go func() {
		defer wg.Done()
		err := suite.srv.Run()
		suite.Require().Error(err)
		suite.Require().ErrorIs(err, errTestDone)
	}()

	suite.waitStatus(lbtypes.ServiceRunning)

	stopCtx, cancel := context.WithTimeout(context.Background(), time.Millisecond*100)
	defer cancel()
	err := suite.srv.Stop(stopCtx)
	suite.Require().NoError(err)
	wg.Wait()

	suite.mock.AssertExpectations(suite.T())
}

func (suite *mockServiceSuite) TestSuspend() {
	ev := suite.mock.makeEventsMocks(false)

	suite.mock.On("Shutdown").Return().Run(func(mock.Arguments) {
		close(ev)
	})
	suite.mock.On("Err").Return(nil).Once()
	suite.mock.On("Err").Return(errFatal.Wrap(errTestDone)).Once()

	wg := sync.WaitGroup{}
	wg.Add(1)

	go func() {
		defer wg.Done()
		err := suite.srv.Run()
		suite.Require().Error(err)
		suite.Require().ErrorIs(err, errTestDone)
	}()

	suite.waitStatus(lbtypes.ServiceRunning)

	suite.srv.Suspend(0)

	suite.waitStatus(lbtypes.ServiceSuspended)

	suite.Require().NoError(suite.srv.Resume())

	wg.Wait()
}

func (suite *mockServiceSuite) waitStatus(st lbtypes.ServiceStatus) {
	for i := 0; i < 100; i++ {
		if suite.srv.status == st {
			return
		}
		time.Sleep(time.Millisecond * 5)
	}

	panic(fmt.Sprintf("service is in incorrect status: %v", suite.srv.status))
}

type serviceSuite struct {
	// NOTE: this suite is very slow (approx 300 ms on one test)
	lbBaseSuite
	srvHelper
}

func TestService(t *testing.T) {
	suite.Run(t, new(serviceSuite))
}

func (suite *serviceSuite) SetupTest() {
	suite.lbBaseSuite.SetupTest()
	suite.setupService(&suite.Suite, suite.lb.Address(),
		Endpoint(suite.lbEndpoint, suite.lbPort),
		Logger(suite.logger),
		ConsumeTopic("lb_read_service", suite.lbTopic),
	)
}

func (suite *serviceSuite) TearDownTest() {
	defer suite.ctxCancel()
	suite.teardownService()
	suite.lbBaseSuite.TearDownTest()
}

func (suite *serviceSuite) TestMessageRead() {
	messages := make([]string, 6)
	for i := range messages {
		messages[i] = fmt.Sprintf("message-%d", i)
	}

	// NOTE: fake lb ignores messages read limits
	written := 0
	wm := make([][]persqueue.WriteMessage, len(messages)/2)
	for i := 0; i < len(messages)/2; i++ {
		wm[i] = make([]persqueue.WriteMessage, 2)
		wm[i][0].Data = []byte(messages[i*2])
		wm[i][1].Data = []byte(messages[i*2+1])
	}

	suite.write(wm[written])
	written++

	suite.srv.cfg.readLimit = 2
	suite.srv.queueOverride = persqueue.NewReader

	var gotSources []lbtypes.SourceID
	var gotMessages []lbtypes.ReadMessage
	var gotStats lbtypes.ServiceCounters

	suite.handler = func(_ context.Context, src lbtypes.SourceID, msg *lbtypes.Messages) {
		gotSources = append(gotSources, src)
		gotMessages = append(gotMessages, msg.Messages...)

		if written >= len(wm) {
			time.Sleep(time.Millisecond * 10) // async stats
			gotStats = suite.srv.Stats()
			go func() {
				_ = suite.srv.Stop(suite.ctx)
			}()
			return
		}

		suite.write(wm[written])
		written++
	}

	err := suite.srv.Run()
	suite.Require().NoError(err)

	suite.Require().Len(gotSources, 3)
	suite.Require().EqualValues("source:0", gotSources[0])

	suite.Require().Len(gotMessages, len(messages))
	for i := range messages {
		data, err := io.ReadAll(gotMessages[i])
		suite.Require().NoError(err)
		suite.EqualValues(messages[i], data)
	}

	suite.EqualValues(lbtypes.ServiceRunning, gotStats.Status)
	suite.EqualValues(2, gotStats.InflyMessages)
	suite.EqualValues(6, gotStats.ReadMessages)
	suite.EqualValues(3, gotStats.HandleCalls)
	suite.EqualValues(2, gotStats.Handled) // stats got from last handle
	suite.EqualValues(1, gotStats.Locks)
	suite.EqualValues(1, gotStats.ActiveLocks)
	suite.NotZero(gotStats.ReaderStats)
}

// Helper funcs for interfaces

type srvHelper struct {
	srv     *ConsumerService
	handler func(context.Context, lbtypes.SourceID, *lbtypes.Messages)
	offset  func(lbtypes.SourceID) (uint64, error)
}

func (h *srvHelper) setupService(suite *suite.Suite, lbAddress string, options ...ConsumerOption) {
	h.handler = nil
	h.offset = nil

	var err error
	h.srv, err = NewConsumerService(h, h, options...)
	suite.Require().NoError(err)

	h.srv.cfg.ReaderOptions = h.srv.cfg.ReaderOptions.WithProxy(lbAddress)
}

func (h *srvHelper) teardownService() {
	ctx, cancel := context.WithTimeout(context.Background(), time.Millisecond*10)
	defer cancel()
	_ = h.srv.Stop(ctx)
}

func (h *srvHelper) Handle(ctx context.Context, src lbtypes.SourceID, msg *lbtypes.Messages) {
	if h.handler != nil {
		h.handler(ctx, src, msg)
	}
}

func (h *srvHelper) GetOffset(_ context.Context, s lbtypes.SourceID) (uint64, error) {
	if h.offset != nil {
		return h.offset(s)
	}
	return 0, nil
}
