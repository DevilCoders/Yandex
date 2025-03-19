package lockbox

import (
	"context"
	"io"
	"net/http"
	"strings"
	"testing"

	"github.com/stretchr/testify/suite"
)

type getSecretTestSuite struct {
	suite.Suite

	client *Client

	data    string
	code    int
	callURL string
}

func TestGetSecret(t *testing.T) {
	suite.Run(t, new(getSecretTestSuite))
}

func (suite *getSecretTestSuite) SetupTest() {
	suite.client = &Client{
		endpoint:   "http://lock-box/",
		auth:       nil,
		httpClient: suite,
	}

	suite.data = ""
	suite.code = 200
	suite.callURL = ""
}

func (suite *getSecretTestSuite) Do(req *http.Request) (*http.Response, error) {
	suite.callURL = req.URL.String()
	return &http.Response{
		StatusCode: suite.code,
		Body:       io.NopCloser(strings.NewReader(suite.data)),
	}, nil
}

func (suite *getSecretTestSuite) TestParse() {
	suite.data = `
{
 "entries": [
  {
   "key": "yaml",
   "binaryValue": "a2V5OiB2YWx1ZQpvYmplY3Q6CiAgd2l0aDogdmFsdWUK"
  },
  {
   "key": "piper.key.with.dot",
   "textValue": "some text here"
  }
 ],
 "versionId": "secret-version-id"
}`
	secret, err := suite.client.GetSecret(context.TODO(), "my-secret-id")

	suite.Require().NoError(err)
	suite.Equal("http://lock-box/lockbox/v1/secrets/my-secret-id/payload", suite.callURL)
	suite.Equal("some text here", secret["piper.key.with.dot"].Text)
	suite.Equal("key: value\nobject:\n  with: value\n", string(secret["yaml"].Data))
}

func (suite *getSecretTestSuite) TestNotFound() {
	suite.code = 404
	_, err := suite.client.GetSecret(context.TODO(), "my-secret-id")

	suite.Require().ErrorIs(err, ErrNotFound)
}

func (suite *getSecretTestSuite) TestServiceBroken() {
	suite.code = 500
	_, err := suite.client.GetSecret(context.TODO(), "my-secret-id")

	suite.Require().ErrorIs(err, ErrLockboxServiceBroken)
}
