package types

import (
	"encoding/json"
	"os"
	"path"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/library/go/test/yatest"
)

var testDataPath = yatest.SourcePath("cloud/billing/go/piper/internal/types/test-data/")

type metricsTestSuite struct {
	suite.Suite
}

func TestDumpErrors(t *testing.T) {
	suite.Run(t, new(metricsTestSuite))
}

func (suite *metricsTestSuite) TestEnrichedQueueMetricUnmarshal() {
	f, err := os.Open(path.Join(testDataPath, "EnrichedQueueMetric-samples.json-lines"))
	if !suite.NoError(err) {
		return
	}
	dec := json.NewDecoder(f)
	dec.DisallowUnknownFields()
	for dec.More() {
		var v EnrichedQueueMetric
		suite.NoError(dec.Decode(&v))
	}
}
