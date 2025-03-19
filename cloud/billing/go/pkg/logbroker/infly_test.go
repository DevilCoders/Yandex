package logbroker

import (
	"context"
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
)

type inflySuite struct {
	suite.Suite

	ctx       context.Context
	ctxCancel context.CancelFunc

	store inflyMessages

	src sourceKey
}

func TestInfly(t *testing.T) {
	suite.Run(t, new(inflySuite))
}

func (suite *inflySuite) SetupTest() {
	suite.ctx, suite.ctxCancel = context.WithCancel(context.Background())

	suite.store = inflyMessages{}
	suite.store.resetInfly()

	suite.src = sourceKey{topic: "topic", partition: 99}
}

func (suite *inflySuite) TearDownTest() {
	suite.ctxCancel()
}

func (suite *inflySuite) TestSourceRegister() {
	err := suite.store.registerSource(suite.ctx, suite.src)
	suite.Require().NoError(err)

	suite.Require().Contains(suite.store.sources, suite.src)
	suite.Contains(suite.store.messages, suite.src)

	srcCtx := suite.store.sources[suite.src]
	suite.Require().NoError(srcCtx.ctx.Err())
	srcCtx.cancel()
	suite.Require().Error(srcCtx.ctx.Err())
}

func (suite *inflySuite) TestSourceCancelByRoot() {
	err := suite.store.registerSource(suite.ctx, suite.src)
	suite.Require().NoError(err)

	srcCtx := suite.store.sources[suite.src]
	suite.ctxCancel()

	suite.Require().Error(srcCtx.ctx.Err())
}

func (suite *inflySuite) TestReleaseSource() {
	err := suite.store.registerSource(suite.ctx, suite.src)
	suite.Require().NoError(err)

	err = suite.store.pushData(&committerMock{}, []sourceMessages{{
		src:      suite.src,
		messages: []persqueue.ReadMessage{{Offset: 1}},
	}})
	suite.Require().NoError(err)

	srcCtx := suite.store.sources[suite.src]

	err = suite.store.releaseSource(suite.src)
	suite.Require().Error(err)

	_, _ = suite.store.getHandlingParams()
	suite.Require().Error(err)

	err = suite.store.releaseSource(suite.src)
	suite.Require().NoError(err)

	suite.Error(srcCtx.ctx.Err())
	suite.NotContains(suite.store.sources, suite.src)
	suite.NotContains(suite.store.messages, suite.src)
}

func (suite *inflySuite) TestData() {
	err := suite.store.registerSource(suite.ctx, suite.src)
	suite.Require().NoError(err)

	messages1 := []persqueue.ReadMessage{
		{Offset: 1},
		{Offset: 2},
		{Offset: 3},
	}
	err = suite.store.pushData(&committerMock{}, []sourceMessages{{
		src:      suite.src,
		messages: messages1,
	}})
	suite.Require().NoError(err)

	messages2 := []persqueue.ReadMessage{
		{Offset: 4},
	}
	err = suite.store.pushData(&committerMock{}, []sourceMessages{{
		src:      suite.src,
		messages: messages2,
	}})
	suite.Require().NoError(err)

	suite.ElementsMatch(suite.store.messages[suite.src], append(messages1, messages2...))
}

func (suite *inflySuite) TestLimit() {
	err := suite.store.registerSource(suite.ctx, suite.src)
	suite.Require().NoError(err)

	data := []byte{'x'}
	messages := make([]persqueue.ReadMessage, 99)
	for i := range messages {
		messages[i].Offset = uint64(i)
		messages[i].Data = data
	}

	err = suite.store.pushData(&committerMock{}, []sourceMessages{{
		src:      suite.src,
		messages: messages,
	}})
	suite.Require().NoError(err)

	suite.False(suite.store.checkLimit(100, 100))
	suite.False(suite.store.checkLimit(100, 0))
	suite.True(suite.store.checkLimit(100, 99))
	suite.True(suite.store.checkLimit(99, 100))
	suite.True(suite.store.checkLimit(99, 99))
}

func (suite *inflySuite) TestSize() {
	suite.Zero(suite.store.size())

	src1 := suite.src
	src2 := src1
	src2.partition++

	{
		src := src1
		messages := make([]persqueue.ReadMessage, 99)
		for i := range messages {
			messages[i].Offset = uint64(i)
		}
		err := suite.store.registerSource(suite.ctx, src)
		suite.Require().NoError(err)
		err = suite.store.pushData(&committerMock{}, []sourceMessages{{
			src:      src,
			messages: messages,
		}})
		suite.Require().NoError(err)
	}

	suite.Equal(99, suite.store.size())

	{
		src := src2
		messages := make([]persqueue.ReadMessage, 1)
		for i := range messages {
			messages[i].Offset = uint64(i)
		}
		err := suite.store.registerSource(suite.ctx, src)
		suite.Require().NoError(err)
		err = suite.store.pushData(&committerMock{}, []sourceMessages{{
			src:      src,
			messages: messages,
		}})
		suite.Require().NoError(err)
	}
	suite.Equal(100, suite.store.size())

	suite.store.resetInfly()
	suite.Zero(suite.store.size())
}

func (suite *inflySuite) TestHandlingParams() {
	err := suite.store.registerSource(suite.ctx, suite.src)
	suite.Require().NoError(err)

	src1 := suite.src
	src1.partition++
	src2 := src1
	src2.partition++

	{
		src := src1
		messages := make([]persqueue.ReadMessage, 1)
		for i := range messages {
			messages[i].Offset = uint64(i)
		}
		err := suite.store.registerSource(suite.ctx, src)
		suite.Require().NoError(err)
		err = suite.store.pushData(&committerMock{}, []sourceMessages{{
			src:      src,
			messages: messages,
		}})
		suite.Require().NoError(err)
	}

	{
		src := src2
		messages := make([]persqueue.ReadMessage, 1)
		for i := range messages {
			messages[i].Offset = uint64(i)
		}
		err := suite.store.registerSource(suite.ctx, src)
		suite.Require().NoError(err)
		err = suite.store.pushData(&committerMock{}, []sourceMessages{{
			src:      src,
			messages: messages,
		}})
		suite.Require().NoError(err)
	}

	src1Messages := suite.store.messages[src1]
	src2Messages := suite.store.messages[src2]

	params, commiters := suite.store.getHandlingParams()

	suite.Require().Len(params, 2)
	suite.Require().Len(commiters, 2)

	p1, p2 := params[0], params[1]
	if p1.src != src1 {
		p1, p2 = p2, p1
	}

	suite.Equal(src1, p1.src)
	suite.Equal(suite.store.sources[src1].ctx, p1.ctx)
	suite.ElementsMatch(src1Messages, p1.messages)
	suite.Empty(suite.store.messages[src1])

	suite.Equal(src2, p2.src)
	suite.Equal(suite.store.sources[src2].ctx, p2.ctx)
	suite.ElementsMatch(src2Messages, p2.messages)
	suite.Empty(suite.store.messages[src2])

	suite.Empty(suite.store.commiters)
}

func (suite *inflySuite) TestCommit() {
	cm := &committerMock{}

	cm.On("Commit").Twice()

	err := suite.store.registerSource(suite.ctx, suite.src)
	suite.Require().NoError(err)

	err = suite.store.pushData(cm, []sourceMessages{{
		src:      suite.src,
		messages: make([]persqueue.ReadMessage, 1),
	}})
	suite.Require().NoError(err)

	err = suite.store.pushData(cm, []sourceMessages{{
		src:      suite.src,
		messages: make([]persqueue.ReadMessage, 1),
	}})
	suite.Require().NoError(err)

	_, commiters := suite.store.getHandlingParams()
	for _, c := range commiters {
		c.Commit()
	}

	cm.AssertExpectations(suite.T())

	suite.Empty(suite.store.messages[suite.src])
	suite.Empty(suite.store.commiters)
}

func (suite *inflySuite) TestReject() {
	cm := &committerMock{}

	err := suite.store.registerSource(suite.ctx, suite.src)
	suite.Require().NoError(err)

	err = suite.store.pushData(cm, []sourceMessages{{
		src:      suite.src,
		messages: make([]persqueue.ReadMessage, 1),
	}})
	suite.Require().NoError(err)

	_, _ = suite.store.getHandlingParams()

	cm.AssertNotCalled(suite.T(), "Commit")

	suite.Empty(suite.store.messages[suite.src])
	suite.Empty(suite.store.commiters)
}

func (suite *inflySuite) TestSourceRegisterTwice() {
	err := suite.store.registerSource(suite.ctx, suite.src)
	suite.Require().NoError(err)

	err = suite.store.pushData(&committerMock{}, []sourceMessages{{
		src:      suite.src,
		messages: []persqueue.ReadMessage{{Offset: 1}},
	}})
	suite.Require().NoError(err)

	srcCtx := suite.store.sources[suite.src]

	err = suite.store.registerSource(suite.ctx, suite.src)
	suite.Require().Error(err)

	suite.Error(srcCtx.ctx.Err())

	suite.Contains(suite.store.messages, suite.src)
	suite.Empty(suite.store.messages[suite.src])
}

func (suite *inflySuite) TestReleaseUnlockedSource() {
	err := suite.store.releaseSource(suite.src)
	suite.Require().Error(err)
}

func (suite *inflySuite) TestReset() {
	err := suite.store.registerSource(suite.ctx, suite.src)
	suite.Require().NoError(err)

	err = suite.store.pushData(&committerMock{}, []sourceMessages{{
		src:      suite.src,
		messages: []persqueue.ReadMessage{{Offset: 1}},
	}})
	suite.Require().NoError(err)

	srcCtx := suite.store.sources[suite.src]

	suite.store.resetInfly()
	suite.Error(srcCtx.ctx.Err())
	suite.Empty(suite.store.messages)
}

func (suite *inflySuite) TestDataWithoutSource() {
	err := suite.store.pushData(&committerMock{}, []sourceMessages{{
		src:      suite.src,
		messages: []persqueue.ReadMessage{{Offset: 1}},
	}})
	suite.Require().Error(err)

	suite.Empty(suite.store.messages[suite.src])
}

type committerMock struct {
	mock.Mock
}

func (m *committerMock) Commit() {
	m.Called()
}
