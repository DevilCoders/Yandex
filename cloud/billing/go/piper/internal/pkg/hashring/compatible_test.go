package hashring

import (
	"encoding/json"
	"fmt"
	"io"
	"os"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/library/go/test/yatest"
)

/*
Use `cd gentest && ya make . && ./gentest > goldenset.json` for testcases update
*/

type testCase struct {
	Key   string
	Node  string
	Parts int
}

type compatibleTestSuite struct {
	suite.Suite

	cases []testCase
}

func TestCompatible(t *testing.T) {
	suite.Run(t, new(compatibleTestSuite))
}

var (
	gentestPath = yatest.SourcePath("cloud/billing/go/piper/internal/pkg/hashring/gentest/goldenset.json")
)

func (suite *compatibleTestSuite) SetupSuite() {
	f, err := os.Open(gentestPath)
	suite.Require().NoError(err)

	dec := json.NewDecoder(f)

	for {
		var tc testCase
		err := dec.Decode(&tc)
		if err == io.EOF {
			break
		}
		suite.Require().NoError(err)

		suite.cases = append(suite.cases, tc)
	}
	suite.Require().NotEmpty(suite.cases)
}

func (suite *compatibleTestSuite) TestCompatibility() {
	for _, c := range suite.cases {
		rng := NewCompatible("realtime", c.Parts)
		partition, _ := rng.GetPartition(c.Key)
		node := fmt.Sprintf("realtime.%d", partition)
		suite.Require().Equal(c.Node, node, "key='%s' partitions=%d", c.Key, c.Parts)
	}
}
