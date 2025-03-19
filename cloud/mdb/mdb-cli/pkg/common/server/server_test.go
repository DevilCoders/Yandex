package server

import (
	"context"
	"fmt"
	"net/http"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/core/log/nop"
)

func TestServer(t *testing.T) {
	token, err := GetToken(context.Background(), testURLRequest, "", &nop.Logger{})
	require.NoError(t, err)
	require.NotNil(t, token)
	require.NotNil(t, token.ExpiresAt)
	require.NoError(t, token.Err)
	assert.Equal(t, token.IamToken, testIAMToken)
	expireTime := time.Unix(token.ExpiresAt.Seconds, int64(token.ExpiresAt.Nanos)).UTC()
	assert.Equal(t, expireTime, testTokenExpireTime)
}

const testIAMToken = "TOKEN!!!"

var testTokenExpireTime = time.Date(2019, 10, 30, 12, 40, 35, 0, time.UTC)

func testURLRequest(serverAddress string) error {
	req, err := http.NewRequest("GET", fmt.Sprintf("http://%s/", serverAddress), nil)
	if err != nil {
		return err
	}

	q := req.URL.Query()
	q.Add("token", testIAMToken)
	q.Add("expiresAt", testTokenExpireTime.Format(time.RFC3339))
	req.URL.RawQuery = q.Encode()
	_, err = (&http.Client{}).Do(req)
	return err
}
