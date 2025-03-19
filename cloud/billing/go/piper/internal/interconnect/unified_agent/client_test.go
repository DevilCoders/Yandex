package unifiedagent

import (
	"context"
	"encoding/json"
	"net/http"
	"testing"

	"github.com/go-resty/resty/v2"
	"github.com/jarcoal/httpmock"
	"github.com/stretchr/testify/suite"
)

type pushMetricsTestSuite struct {
	suite.Suite

	client *uaClient
	metric SolomonMetric
}

func TestPushMetrics(t *testing.T) {
	suite.Run(t, new(pushMetricsTestSuite))
}

func (suite *pushMetricsTestSuite) SetupTest() {
	suite.client = &uaClient{
		healthCheckPort:    12345,
		solomonMetricsPort: 12346,
		httpClient:         resty.New(),
	}
	suite.metric = SolomonMetric{
		Labels:    map[string]string{"test-label": "test-value"},
		Value:     json.Number("10.12345"),
		Timestamp: 1000,
	}

	httpmock.ActivateNonDefault(suite.client.httpClient.GetClient())
}

func (suite *pushMetricsTestSuite) TearDownTest() {
	httpmock.DeactivateAndReset()
}

func (suite *pushMetricsTestSuite) TestPostMetric() {
	httpmock.RegisterResponder("POST", "http://localhost:12346/write", func(req *http.Request) (*http.Response, error) {
		metric := make(map[string]interface{})
		if err := json.NewDecoder(req.Body).Decode(&metric); err != nil {
			return httpmock.NewStringResponse(400, ""), nil
		}

		resp, err := httpmock.NewJsonResponse(200, metric)
		if err != nil {
			return httpmock.NewStringResponse(500, ""), nil
		}
		return resp, nil
	})

	err := suite.client.PushMetrics(context.TODO(), suite.metric)

	suite.Require().NoError(err)
	suite.Equal(1, httpmock.GetTotalCallCount())

	info := httpmock.GetCallCountInfo()
	suite.Equal(1, info["POST http://localhost:12346/write"])
}

func (suite *pushMetricsTestSuite) TestRequestValueType() {
	var metrics struct {
		Metrics []struct {
			Value json.RawMessage
		}
	}

	httpmock.RegisterResponder("POST", "http://localhost:12346/write", func(req *http.Request) (*http.Response, error) {
		if err := json.NewDecoder(req.Body).Decode(&metrics); err != nil {
			panic(err)
		}

		return httpmock.NewStringResponse(200, ""), nil
	})

	_ = suite.client.PushMetrics(context.TODO(), suite.metric)

	suite.Require().Len(metrics.Metrics, 1)
	suite.Equal(suite.metric.Value.String(), string(metrics.Metrics[0].Value)) // No quotes = good number
}

func (suite *pushMetricsTestSuite) TestServiceBroken() {
	httpmock.RegisterResponder("POST", "http://localhost:12346/write", func(req *http.Request) (*http.Response, error) {
		return httpmock.NewStringResponse(500, ""), nil
	})

	err := suite.client.PushMetrics(context.TODO(), suite.metric)

	suite.Require().Error(err)
}
