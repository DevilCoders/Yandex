package httpyav

import (
	"bytes"
	"crypto/rsa"
	"crypto/x509"
	"encoding/json"
	"encoding/pem"
	"net/http"
	"strconv"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yandex/yav"
)

func TestRSASignRequest(t *testing.T) {
	testCases := []struct {
		request *http.Request
		login   string
		privKey *rsa.PrivateKey
	}{
		{
			request: func() *http.Request {
				body := yav.GetSecretsRequest{Tags: []string{"tag1", "tag2"}, Query: "тестовый секрет"}
				b, err := json.Marshal(body)
				require.NoError(t, err)

				req, err := http.NewRequest("GET", "http://vault-api.passport.yandex.net/1/secrets/", bytes.NewReader(b))
				require.NoError(t, err)

				return req
			}(),
			login: "ppodolsky",
			privKey: func() *rsa.PrivateKey {
				privPem, _ := pem.Decode([]byte(testPrivateRSAKey))
				pk, err := x509.ParsePKCS1PrivateKey(privPem.Bytes)
				require.NoError(t, err)
				return pk
			}(),
		},
	}

	for _, tc := range testCases {
		t.Run("", func(t *testing.T) {
			err := rsaSignRequest(tc.request, tc.login, tc.privKey)
			assert.NoError(t, err)

			assert.Contains(t, tc.request.Header, "X-Ya-Rsa-Signature")
			assert.Contains(t, tc.request.Header, "X-Ya-Rsa-Login")
			assert.Contains(t, tc.request.Header, "X-Ya-Rsa-Timestamp")

			assert.Equal(t, tc.request.Header.Get("X-Ya-Rsa-Login"), tc.login)

			// compare signatures
			timestamp, err := strconv.ParseInt(tc.request.Header.Get("X-Ya-Rsa-Timestamp"), 10, 64)
			assert.NoError(t, err)

			payload, err := json.Marshal(tc.request.Body)
			assert.NoError(t, err)

			serialized := serializeRequest(
				tc.request.Method,
				tc.request.URL.Path,
				payload,
				time.Unix(timestamp, 0),
				tc.login,
			)

			sig, err := computeRSASignature(serialized, tc.privKey)
			assert.NoError(t, err)

			assert.Equal(t, sig, tc.request.Header.Get("X-Ya-Rsa-Signature"))
		})
	}
}

func TestSerializeRequest(t *testing.T) {
	testCases := []struct {
		name         string
		method       string
		path         string
		payload      []byte
		ts           time.Time
		login        string
		expectResult string
	}{
		{
			name:         "no_payload",
			method:       "get",
			path:         "http://vault-api.passport.yandex.net/1/secrets/",
			payload:      nil,
			ts:           time.Unix(1565681543, 0),
			login:        "ppodolsky",
			expectResult: "GET\nhttp://vault-api.passport.yandex.net/1/secrets/\n\n1565681543\nppodolsky\n",
		},
		{
			name:    "with_payload",
			method:  "get",
			path:    "http://vault-api.passport.yandex.net/1/secrets/",
			payload: []byte(`{"tags": "tag1,tag2", "query": "тестовый секрет"}`),
			ts:      time.Unix(1565681543, 0),
			login:   "ppodolsky",
			expectResult: "GET\nhttp://vault-api.passport.yandex.net/1/secrets/\n{\"tags\": \"tag1,tag2\", " +
				"\"query\": \"\xd1\x82\xd0\xb5\xd1\x81\xd1\x82\xd0\xbe\xd0\xb2\xd1\x8b\xd0\xb9 " +
				"\xd1\x81\xd0\xb5\xd0\xba\xd1\x80\xd0\xb5\xd1\x82\"}\n1565681543\nppodolsky\n",
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			res := serializeRequest(tc.method, tc.path, tc.payload, tc.ts, tc.login)
			assert.Equal(t, tc.expectResult, res)
		})
	}
}
