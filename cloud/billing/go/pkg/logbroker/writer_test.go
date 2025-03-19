package logbroker

import (
	"context"
	"errors"
	"fmt"
	"strings"
	"testing"
	"time"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"
	"golang.org/x/sync/semaphore"

	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue/fake/data"
)

func dummyCallback(error) {}

type mockWriterSuite struct {
	suite.Suite

	ctx    context.Context
	cancel context.CancelFunc

	w *producerFabric

	mock lbWriterMock
}

func TestMockWriter(t *testing.T) {
	suite.Run(t, new(mockWriterSuite))
}

func (suite *mockWriterSuite) SetupTest() {
	suite.ctx, suite.cancel = context.WithTimeout(context.Background(), time.Second)

	suite.mock.initMock()

	suite.w = &producerFabric{
		route:           "route",
		shardPartitions: 2,
		maxMessageSize:  0,
		sem:             semaphore.NewWeighted(2),
		queueOverride: func(o persqueue.WriterOptions) persqueue.Writer {
			w := suite.mock.withSource(string(o.SourceID))
			w.partition = 999
			if o.PartitionGroup != 0 {
				w.partition = uint64(o.PartitionGroup - 1)
			}
			w.topic = o.Topic
			return w
		},
	}
}

func (suite *mockWriterSuite) TearDownTest() {
	suite.cancel()
}

func (suite *mockWriterSuite) TestConstructorErrors() {
	cases := []struct {
		cfg             persqueue.WriterOptions
		route           string
		shardPartitions int
		maxMessageSize  int
		maxParallel     int
		wantError       bool
	}{
		{persqueue.WriterOptions{}, "route", 99, 0, 0, false},
		{persqueue.WriterOptions{PartitionGroup: 1}, "route", 99, 0, 0, true},
		{persqueue.WriterOptions{SourceID: []byte("si")}, "route", 99, 0, 0, true},
		{persqueue.WriterOptions{}, "", 99, 0, 0, true},
		{persqueue.WriterOptions{}, "route", 0, 0, 0, true},
	}

	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			w, err := NewShardProducer(c.cfg, c.route, c.shardPartitions, c.maxMessageSize, c.maxParallel)
			if !c.wantError {
				suite.Require().NoError(err)
				suite.Require().NotNil(w)
				return
			}
			suite.Require().Error(err)
			suite.Require().ErrorIs(err, ErrMissconfigured)
		})
	}
}

func (suite *mockWriterSuite) TestConstructorParallel() {
	w, err := NewShardProducer(persqueue.WriterOptions{}, "x", 9, 0, 0)
	suite.Require().NoError(err)
	pf := w.(*producerFabric)

	suite.True(pf.sem.TryAcquire(2))
}

func (suite *mockWriterSuite) TestGetOffset() {
	cases := []struct {
		seq1, seq2 int
		wantOffset int // offset is less than sequence by 1
	}{
		{0, 0, 0},
		{42, 0, 41},
		{42, 0, 41},
		{42, 99, 41},
	}
	suite.mock.On("Close", "13/route.0/logbroker-grpc/offsets").Return(nil)
	suite.mock.On("Close", "13/route.1/logbroker-grpc/offsets").Return(nil)

	for _, c := range cases {
		suite.Run(fmt.Sprintf("offsets: %d %d", c.seq1, c.seq2), func() {
			suite.mock.On("Init", "13/route.0/logbroker-grpc/offsets").Once().Return(c.seq1, nil)
			suite.mock.On("Init", "13/route.1/logbroker-grpc/offsets").Once().Return(c.seq2, nil)
			off, err := suite.w.GetOffset(suite.ctx, lbtypes.SourceID("offsets"))
			suite.Require().NoError(err)
			suite.EqualValues(c.wantOffset, off)
		})
	}
}

func (suite *mockWriterSuite) TestGetOffsetError() {
	suite.mock.On("Close", "13/route.0/logbroker-grpc/offsets").Return(nil)
	suite.mock.On("Close", "13/route.1/logbroker-grpc/offsets").Return(nil)

	suite.mock.On("Init", "13/route.0/logbroker-grpc/offsets").Once().Return(0, errors.New("test error"))
	suite.mock.On("Init", "13/route.1/logbroker-grpc/offsets").Once().Return(42, nil)

	_, err := suite.w.GetOffset(suite.ctx, lbtypes.SourceID("offsets"))
	suite.Error(err)
}

func (suite *mockWriterSuite) TestGetOffsetSinglePartition() {
	suite.mock.On("Close", "13/route/logbroker-grpc/offsets").Return(nil)
	suite.mock.On("Init", "13/route/logbroker-grpc/offsets").Once().Return(42, nil)

	suite.w.shardPartitions = 1

	off, err := suite.w.GetOffset(suite.ctx, lbtypes.SourceID("offsets"))
	suite.Require().NoError(err)
	suite.EqualValues(41, off)
}

func (suite *mockWriterSuite) TestWrite() {
	sourceName := "13/route.0/logbroker-grpc/write"

	acks := make(chan persqueue.WriteResponse, 1)
	acks <- &persqueue.Ack{SeqNo: 42}
	close(acks)

	suite.mock.On("Init", sourceName).Return(2, nil)
	suite.mock.On("Close", sourceName).Return(nil)
	suite.mock.On("C").Once().Return(acks)
	suite.mock.On("Write", sourceName, "1\n2\n").Once().Return(nil)

	src := lbtypes.SourceID("write")
	off, err := suite.w.Write(suite.ctx, src, 0, []lbtypes.ShardMessage{
		&shrdMessage{"0", 1},
		&shrdMessage{"1", 2},
		&shrdMessage{"2", 3},
	})

	suite.Require().NoError(err)
	suite.EqualValues(41, off) // this should be taken from acks so not equal messages offset
}

func (suite *mockWriterSuite) TestWritePartitionError() {
	src := lbtypes.SourceID("write")
	_, err := suite.w.Write(suite.ctx, src, uint32(suite.w.shardPartitions), nil)

	suite.Require().Error(err)
	suite.ErrorIs(err, ErrWrite)
}

func (suite *mockWriterSuite) TestWriteInitError() {
	sourceName := "13/route.0/logbroker-grpc/write"
	suite.mock.On("Init", sourceName).Return(0, errors.New("test init error"))

	src := lbtypes.SourceID("write")
	_, err := suite.w.Write(suite.ctx, src, 0, nil)

	suite.Require().Error(err)
	suite.ErrorIs(err, ErrWriterInit)
}

func (suite *mockWriterSuite) TestWriteErrorIssues() {
	sourceName := "13/route.0/logbroker-grpc/write"

	acks := make(chan persqueue.WriteResponse, 1)
	acks <- &persqueue.Issue{Err: errors.New("ISSUE")}
	close(acks)

	suite.mock.On("Init", sourceName).Return(0, nil)
	suite.mock.On("Close", sourceName).Return(nil)
	suite.mock.On("C").Once().Return(acks)
	suite.mock.On("Write", sourceName, mock.Anything).Once().Return(errors.New("test write error"))

	src := lbtypes.SourceID("write")
	_, err := suite.w.Write(suite.ctx, src, 0, []lbtypes.ShardMessage{
		&shrdMessage{"1", 1},
	})

	suite.Require().Error(err)
	suite.ErrorIs(err, ErrWrite)

	issues := GetWriteIssues(err)
	suite.EqualValues([]*persqueue.Issue{{Err: errors.New("ISSUE")}}, issues)
}

func (suite *mockWriterSuite) TestWriteCloseError() {
	sourceName := "13/route.0/logbroker-grpc/write"

	acks := make(chan persqueue.WriteResponse)
	close(acks)

	suite.mock.On("Init", sourceName).Return(0, nil)
	suite.mock.On("Close", sourceName).Return(errors.New("test close error"))
	suite.mock.On("C").Once().Return(acks)
	suite.mock.On("Write", sourceName, mock.Anything).Once().Return(nil)

	src := lbtypes.SourceID("write")
	_, err := suite.w.Write(suite.ctx, src, 0, []lbtypes.ShardMessage{
		&shrdMessage{"1", 1},
	})

	suite.Require().Error(err)
	suite.ErrorIs(err, ErrWrite)
}

func (suite *mockWriterSuite) TestPartitionsCount() {
	cnt := suite.w.PartitionsCount()
	suite.EqualValues(2, cnt)
}

func (suite *mockWriterSuite) TestSourceID() {
	const route = "thisIsRoute"
	const sourceName = "andSource:name"
	si := suite.w.buildSourceID(route, sourceName)

	chunks := strings.Split(si, "/")
	suite.EqualValues([]string{"13", route, "logbroker-grpc", sourceName}, chunks)
}

func (suite *mockWriterSuite) TestShardedSourceID() {
	const route = "thisIsRoute"
	const sourceName = "andSource:name"
	si := suite.w.buildPartitionSourceID(route, 42, sourceName)

	chunks := strings.Split(si, "/")
	suite.EqualValues([]string{"13", route + ".42", "logbroker-grpc", sourceName}, chunks)
}

func (suite *mockWriterSuite) TestPrepareDataEmpty() {
	ch := suite.w.prepareData(suite.ctx, false, 0, nil, dummyCallback)
	got := dumpWriteMessages(ch)

	suite.Empty(got)
}

func (suite *mockWriterSuite) TestPrepareDataUnlimited() {
	ch := suite.w.prepareData(suite.ctx, false, 0, []lbtypes.ShardMessage{
		&shrdMessage{"1\n", 1},
		&shrdMessage{"2", 2},
	}, dummyCallback)

	got := dumpWriteMessages(ch)
	want := []writeMessage{
		{data: []byte("1\n2\n"), offset: 2},
	}
	suite.Require().EqualValues(want, got)
}

func (suite *mockWriterSuite) TestPrepareDataUnlimitedFiltered() {
	ch := suite.w.prepareData(suite.ctx, true, 1, []lbtypes.ShardMessage{
		&shrdMessage{"1\n", 1},
		&shrdMessage{"2", 2},
	}, dummyCallback)

	got := dumpWriteMessages(ch)
	want := []writeMessage{
		{data: []byte("2\n"), offset: 2},
	}
	suite.EqualValues(want, got)
}

func (suite *mockWriterSuite) TestPrepareDataLimited() {
	suite.w.maxMessageSize = 19 // <two messages size> - 1
	ch := suite.w.prepareData(suite.ctx, false, 0, []lbtypes.ShardMessage{
		// Each message is 9 bytes long and \n add 1 to result message
		&shrdMessage{"111111111", 1},
		&shrdMessage{"222222222", 2}, // this message does not fit with line end
		&shrdMessage{"333333333", 3},
		&shrdMessage{"444444444", 3}, // this message does not fit by size but have same offset
		&shrdMessage{"555555555", 4},
		&shrdMessage{"6666", 5},
		&shrdMessage{"7777", 6},
		&shrdMessage{"8888", 7},
		&shrdMessage{"9999", 8},
	}, dummyCallback)

	got := dumpWriteMessages(ch)
	want := []writeMessage{
		{data: []byte("111111111\n"), offset: 1},
		{data: []byte("222222222\n"), offset: 2},
		{data: []byte("333333333\n444444444\n"), offset: 3},
		{data: []byte("555555555\n6666\n"), offset: 5},
		{data: []byte("7777\n8888\n9999\n"), offset: 8},
	}
	suite.EqualValues(want, got)
}

func (suite *mockWriterSuite) TestPrepareDataLimitedFiltered() {
	suite.w.maxMessageSize = 10
	ch := suite.w.prepareData(suite.ctx, true, 2, []lbtypes.ShardMessage{
		&shrdMessage{"111111111", 1},
		&shrdMessage{"222222222", 2},
		&shrdMessage{"333333333", 3},
		&shrdMessage{"444444444", 4},
		&shrdMessage{"555555555", 5},
	}, dummyCallback)

	got := dumpWriteMessages(ch)
	want := []writeMessage{
		{data: []byte("333333333\n"), offset: 3},
		{data: []byte("444444444\n"), offset: 4},
		{data: []byte("555555555\n"), offset: 5},
	}
	suite.EqualValues(want, got)
}

func (suite *mockWriterSuite) TestPrepareDataLimitedStopsOnCanceled() {
	suite.w.maxMessageSize = 10
	ch := suite.w.prepareData(suite.ctx, false, 0, []lbtypes.ShardMessage{
		&shrdMessage{"111111111", 1},
		&shrdMessage{"222222222", 2},
	}, dummyCallback)

	time.Sleep(time.Millisecond)
	suite.cancel()
	got := dumpWriteMessages(ch)
	suite.Len(got, 1)
}

type writerSuite struct {
	// NOTE: this suite is very slow (approx 300 ms on one test)
	lbBaseSuite

	w lbtypes.ShardProducer
}

func TestWriter(t *testing.T) {
	suite.Run(t, new(writerSuite))
}

func (suite *writerSuite) SetupTest() {
	suite.lbBaseSuite.SetupTest()

	wo := suite.writerOptions
	wo.PartitionGroup = 0
	wo.SourceID = nil

	var err error
	suite.w, err = NewShardProducer(wo, "test", 2, 1, 0)
	suite.Require().NoError(err)
}

func (suite *writerSuite) TestWrite() {
	source := lbtypes.SourceID("gotest")

	subCtx, subCancel := context.WithCancel(suite.ctx)
	defer subCancel()
	messages := make([]*data.Message, 0, 10000)
	ch := make(chan *data.Message, 10000)
	err := suite.lb.Subscribe(subCtx, "source", ch)
	suite.Require().NoError(err)

	go func() {
		for m := range ch {
			messages = append(messages, m)
		}
	}()

	off, err := suite.w.GetOffset(suite.ctx, source)
	suite.Require().NoError(err)
	suite.EqualValues(0, off)

	// NOTE: Fake does not memoize seqNo. So check only one write separately
	wo, err := suite.w.Write(suite.ctx, source, 0, []lbtypes.ShardMessage{
		&shrdMessage{data: "1", off: 1},
		&shrdMessage{data: "2", off: 2},
		&shrdMessage{data: "3", off: 3},
	})
	suite.Require().NoError(err)
	suite.Require().EqualValues(3, wo)

	subCancel()

	suite.Require().Len(messages, 3)
	for i, m := range messages {
		suite.Require().EqualValues(0, m.Partition)
		suite.Require().EqualValues("13/test.0/logbroker-grpc/gotest", m.Source)
		suite.Require().EqualValues("source", m.Topic)
		suite.Require().EqualValues(i+2, m.SeqNo)
		suite.Require().EqualValues(fmt.Sprintf("%d\n", i+1), m.Data)
	}
}

func BenchmarkSplit(b *testing.B) {
	messages := make([]lbtypes.ShardMessage, 50_000)
	for i := range messages {
		messages[i] = &shrdMessage{
			data: strings.Repeat("A", 1000),
			off:  uint64(i / 10),
		}
	}
	w := producerFabric{
		maxMessageSize: 10 * 1024 * 1024,
	}
	ctx := context.Background()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		ch := w.prepareData(ctx, false, 0, messages, dummyCallback)
		_ = dumpWriteMessages(ch)
	}
}

func BenchmarkSplitToOneMetric(b *testing.B) {
	messages := make([]lbtypes.ShardMessage, 50_000)
	for i := range messages {
		messages[i] = &shrdMessage{
			data: strings.Repeat("A", 1000),
			off:  0,
		}
	}
	w := producerFabric{
		maxMessageSize: 10 * 1024 * 1024,
	}
	ctx := context.Background()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		ch := w.prepareData(ctx, false, 0, messages, dummyCallback)
		_ = dumpWriteMessages(ch)
	}
}

type shrdMessage struct {
	data string
	off  uint64
}

func (m *shrdMessage) Data() ([]byte, error) {
	return []byte(m.data), nil
}

func (m *shrdMessage) Offset() uint64 {
	return m.off
}

func dumpWriteMessages(ch <-chan writeMessage) (result []writeMessage) {
	for m := range ch {
		result = append(result, m)
	}
	return
}
