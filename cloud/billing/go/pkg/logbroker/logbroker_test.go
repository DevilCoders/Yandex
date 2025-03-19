package logbroker

import (
	"context"
	"errors"
	"fmt"
	"strconv"
	"strings"
	"time"

	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zaptest"

	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue/fake"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue/log/corelogadapter"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

type lbBaseSuite struct {
	suite.Suite

	ctx       context.Context
	ctxCancel context.CancelFunc
	logger    log.Logger

	lb            fake.LBFake
	writerOptions persqueue.WriterOptions

	lbEndpoint string
	lbPort     int
	lbTopic    string
}

func (suite *lbBaseSuite) SetupTest() {
	var err error

	suite.ctx, suite.ctxCancel = context.WithTimeout(context.Background(), time.Second*5)
	suite.logger = &zap.Logger{L: zaptest.NewLogger(suite.T())}

	// NOTE: using suite's logger in full tests cause panics on fake stops.
	// suite.lb, err = fake.NewFake(suite.ctx, suite.logger)
	suite.lb, err = fake.NewFake(suite.ctx, corelogadapter.New(nopLogger))
	suite.Require().NoError(err)

	if suite.lbTopic == "" {
		suite.lbTopic = "source"
	}
	err = suite.lb.CreateTopic(suite.lbTopic, 1)
	suite.Require().NoError(err)

	address := suite.lb.Address()
	chunks := strings.Split(address, ":")
	suite.Require().Len(chunks, 2)
	suite.lbEndpoint = chunks[0]
	suite.lbPort, err = strconv.Atoi(chunks[1])
	suite.Require().NoError(err)

	suite.writerOptions = persqueue.WriterOptions{
		Endpoint:       suite.lbEndpoint,
		Port:           suite.lbPort,
		Logger:         corelogadapter.New(suite.logger),
		Topic:          suite.lbTopic,
		SourceID:       []byte(fmt.Sprintf("gotest/%s", uuid.Must(uuid.NewV4()))),
		Codec:          persqueue.Raw,
		RetryOnFailure: true,
	}.WithProxy(address)
}

func (suite *lbBaseSuite) TearDownTest() {
	suite.ctxCancel()
}

func (suite *lbBaseSuite) write(msg []persqueue.WriteMessage) {
	p := persqueue.NewWriter(suite.writerOptions)
	_, err := p.Init(suite.ctx)
	suite.Require().NoError(err)

	defer func() {
		suite.Require().NoError(p.Close())
	}()

	for _, m := range msg {
		m := m
		err := p.Write(&m)
		suite.Require().NoError(err)
	}
}

var errTestDone = errors.New("test done")

type lbReaderMock struct {
	mock.Mock
	closedChan chan struct{}
}

func (m *lbReaderMock) makeEventsMocks(closeEvents bool) (ech chan persqueue.Event) {
	m.closedChan = make(chan struct{})
	ech = make(chan persqueue.Event)
	if closeEvents {
		close(ech)
	}

	m.On("Start").Return("sess", nil)
	m.On("Closed").Return(m.closedChan)
	m.On("C").Return(ech)

	return
}

func (m *lbReaderMock) Start(context.Context) (r *persqueue.ReaderInit, err error) {
	args := m.Called()

	if s := args.String(0); s != "" {
		r = &persqueue.ReaderInit{
			SessionID: s,
		}
	}
	err = args.Error(1)
	return
}

func (m *lbReaderMock) Shutdown() {
	m.Called()
}

func (m *lbReaderMock) Closed() <-chan struct{} {
	return m.Called().Get(0).(chan struct{})
}

func (m *lbReaderMock) Err() error {
	return m.Called().Error(0)
}

func (m *lbReaderMock) C() <-chan persqueue.Event {
	return m.Called().Get(0).(chan persqueue.Event)
}

func (m *lbReaderMock) Stat() persqueue.Stat {
	return m.Called().Get(0).(persqueue.Stat)
}

type lbWriterMock struct {
	*mock.Mock // we want copy this struct but use one mock
	sourceID   string
	topic      string
	partition  uint64
	closedChan chan struct{}
}

func (m *lbWriterMock) initMock() {
	m.Mock = &mock.Mock{}
	m.closedChan = make(chan struct{})
}

func (m lbWriterMock) withSource(s string) *lbWriterMock {
	m.sourceID = s
	return &m
}

func (m *lbWriterMock) Init(context.Context) (init *persqueue.WriterInit, err error) {
	args := m.Called(m.sourceID)

	err = args.Error(1)
	if err == nil {
		init = &persqueue.WriterInit{
			MaxSeqNo:  uint64(args.Int(0)),
			Topic:     m.topic,
			Partition: m.partition,
		}
	}
	return
}

func (m *lbWriterMock) Write(d *persqueue.WriteMessage) error {
	return m.Called(m.sourceID, string(d.Data)).Error(0)
}

func (m *lbWriterMock) Close() error {
	return m.Called(m.sourceID).Error(0)
}

func (m *lbWriterMock) C() <-chan persqueue.WriteResponse {
	return m.Called().Get(0).(chan persqueue.WriteResponse)
}

func (m *lbWriterMock) Closed() <-chan struct{} {
	return m.Called().Get(0).(chan struct{})
}

func (m *lbWriterMock) Stat() persqueue.WriterStat {
	return m.Called().Get(0).(persqueue.WriterStat)
}
