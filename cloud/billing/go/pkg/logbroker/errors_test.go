package logbroker

import (
	"errors"
	"fmt"
	"testing"

	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"github.com/stretchr/testify/suite"
)

type errorsSuite struct {
	suite.Suite
}

func TestErrors(t *testing.T) {
	suite.Run(t, new(errorsSuite))
}

func (suite *errorsSuite) TestWithIssues() {
	iss := withIssues(errors.New("test err"), []*persqueue.Issue{
		{Err: errors.New("testErr")},
	})

	suite.Require().Error(iss)
	suite.Require().IsType((*issuesError)(nil), iss)
}

func (suite *errorsSuite) TestWithIssuesNilErr() {
	iss := withIssues(nil, []*persqueue.Issue{
		{Err: errors.New("testErr")},
	})

	suite.Require().NoError(iss)
}

func (suite *errorsSuite) TestWithIssuesNoIssues() {
	err := errors.New("test err")
	iss := withIssues(err, nil)

	suite.Require().Error(iss)
	suite.Require().Equal(err, iss)
}

func (suite *errorsSuite) TestIssues() {
	err := errors.New("test err")
	issues := []*persqueue.Issue{
		{Err: errors.New("testErr")},
	}

	iss := withIssues(err, issues)

	got := GetWriteIssues(iss)
	suite.Require().Len(got, 1)
	suite.EqualValues(issues[0], got[0])
}

func (suite *errorsSuite) TestIssuesAs() {
	err := errors.New("test err")

	iss := withIssues(err, []*persqueue.Issue{
		{Err: errors.New("testErr")},
	})

	wrapped := fmt.Errorf("wrapped: %w", iss)

	var tgt issuesError

	ok := errors.As(wrapped, &tgt)
	suite.Require().True(ok)
	suite.EqualValues(err, tgt.error)
}

func (suite *errorsSuite) TestIssuesUnwrap() {
	err := errors.New("test err")

	iss := withIssues(err, []*persqueue.Issue{
		{Err: errors.New("testErr")},
	})

	got := errors.Unwrap(iss)
	suite.Require().EqualValues(err, got)
}
