package httpbb_test

import (
	"encoding/json"
	"io/ioutil"
	"net/http"
	"net/http/httptest"
	"net/url"
	"testing"

	"github.com/stretchr/testify/assert"
)

type BlackboxHTTPRsp struct {
	ExpectedMethod string          `json:"EXPECTED_METHOD"`
	ExpectedParams string          `json:"EXPECTED_PARAMS"`
	ExpectedBody   string          `json:"EXPECTED_BODY"`
	BBResponse     json.RawMessage `json:"BB_RESPONSE"`
}

func NewBlackBoxSrv(t *testing.T, rsp BlackboxHTTPRsp) *httptest.Server {
	return httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		assert.Equal(t, "/blackbox", r.URL.Path)

		assert.Equal(t, rsp.ExpectedMethod, r.Method)

		expectedQuery, err := url.ParseQuery(rsp.ExpectedParams)
		if assert.NoError(t, err) {
			assert.EqualValues(t, expectedQuery, r.URL.Query())
		}

		expectedBody, err := url.ParseQuery(rsp.ExpectedBody)
		if assert.NoError(t, err) && len(expectedBody) > 0 {
			body, err := ioutil.ReadAll(r.Body)
			assert.NoError(t, err)

			actualBody, err := url.ParseQuery(string(body))
			assert.NoError(t, err)

			assert.EqualValues(t, expectedBody, actualBody)
		}

		_, _ = w.Write(rsp.BBResponse)
	}))
}
