package errtools

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type StackTestSuite struct {
	suite.Suite
}

func TestStackTestSuite(t *testing.T) {
	suite.Run(t, new(StackTestSuite))
}

func (suite *StackTestSuite) TestWrapErr() {
	err := fmt.Errorf("error")
	wrapped := withCaller(err, 0)

	suite.Require().Contains(wrapped.Error(), "(errtools/stack_test.go (*StackTestSuite).TestWrapErr:")
}

func (suite *StackTestSuite) TestWrapNil() {
	wrapped := withCaller(nil, 0)

	suite.Require().NoError(wrapped)
}

func (suite *StackTestSuite) TestDoubleWrap() {
	err := fmt.Errorf("error")
	wrapped := withCaller(err, 0)

	suite.Require().Equal(wrapped, withCaller(wrapped, 0))
}

func (suite *StackTestSuite) TestPrevWrapErr() {
	var wrapped error
	func() {
		err := fmt.Errorf("error")
		wrapped = WithPrevCaller(err)
	}()

	suite.Require().Contains(wrapped.Error(), "(errtools/stack_test.go (*StackTestSuite).TestPrevWrapErr:")
}

func (suite *StackTestSuite) TestUnwrap() {
	err := fmt.Errorf("error")
	wrapped := withCaller(err, 0)

	suite.Require().Equal(err, xerrors.Unwrap(wrapped))
}

func (suite *StackTestSuite) TestIs() {
	err := fmt.Errorf("error")
	wrapped := withCaller(err, 0)

	suite.Require().True(xerrors.Is(wrapped, err))
}
