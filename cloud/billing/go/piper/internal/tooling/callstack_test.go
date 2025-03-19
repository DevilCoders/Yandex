package tooling

import (
	"testing"

	"github.com/stretchr/testify/suite"
)

type callstackSuite struct {
	suite.Suite
}

func TestCallstack(t *testing.T) {
	suite.Run(t, new(callstackSuite))
}

func CallstackDirectTestFunc() string {
	return getCallFuncName(0)
}

func CallstackIndirectTestFunc() string {
	return getCallFuncName(1)
}

func (suite *callstackSuite) TestDirectFunc() {
	want := "CallstackDirectTestFunc"
	suite.Equal(want, CallstackDirectTestFunc())
}

func (suite *callstackSuite) TestIndirectFunc() {
	want := "TestIndirectFunc"
	suite.Equal(want, CallstackIndirectTestFunc())
}

func BenchmarkCallStack(b *testing.B) {
	for i := 0; i < b.N; i++ {
		fn := CallstackIndirectTestFunc()
		if fn == "" {
			panic(fn)
		}
	}
}
