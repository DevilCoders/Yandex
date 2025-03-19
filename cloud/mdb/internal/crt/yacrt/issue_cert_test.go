package yacrt

import (
	"context"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/crt/fixtures"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/log/nop"
)

var (
	target   = "test.com"
	altNames = []string{"test1.ru", "test2.by"}
	caName   = "InternalCA"
	oauth    = secret.NewString("some_oauth")
)

var configNoRetry = httputil.ClientConfig{
	Transport: httputil.TransportConfig{
		Retry: retry.Config{
			MaxRetries: 1,
		},
	},
}

func Test_crtClient_IssueCertOk(t *testing.T) {
	downloadCalled := false
	downloadServer := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		downloadCalled = true
		w.WriteHeader(http.StatusOK)
		_, err := io.WriteString(w, fixtures.FullPem)
		require.NoError(t, err)
	}))
	defer downloadServer.Close()

	issueCalled := false
	issueServer := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, req *http.Request) {
		issueCalled = true
		w.WriteHeader(issueSuccessCode)
		_, err := io.WriteString(w, fmt.Sprintf(`{"download2":"%s"}`, downloadServer.URL))
		require.NoError(t, err)

		assert.Equal(t, http.MethodPost, req.Method)
		assert.Equal(t, "OAuth "+oauth.Unmask(), req.Header.Get("Authorization"), "invalid authorization header")

		body, _ := ioutil.ReadAll(req.Body)
		bodyStr := string(body)
		assert.Contains(t, bodyStr, target)
		assert.Contains(t, bodyStr, altNames[0])
		assert.Contains(t, bodyStr, altNames[1])
		assert.Contains(t, bodyStr, caName)

	}))
	defer issueServer.Close()

	crt, err := New(&nop.Logger{}, Config{
		ClientConfig: configNoRetry,
		URL:          issueServer.URL,
		OAuth:        oauth,
	})
	require.NoError(t, err)

	cert, err := crt.IssueCert(context.Background(), target, altNames, caName, "host")

	require.NoError(t, err)
	require.NotEmpty(t, cert.CertPem)
	require.NotEmpty(t, cert.KeyPem)
	assert.True(t, downloadCalled, "download not called")
	assert.True(t, issueCalled, "issue new cert not called")
}

func Test_crtClient_IssueCertGetDownloadLinkErrStatus(t *testing.T) {
	issueCalled := false
	issueServer := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		issueCalled = true
		w.WriteHeader(http.StatusInternalServerError)
		_, err := io.WriteString(w, `{}`)
		require.NoError(t, err)
	}))
	defer issueServer.Close()

	crt, err := New(&nop.Logger{}, Config{
		ClientConfig: configNoRetry,
		URL:          issueServer.URL,
		OAuth:        oauth,
	})
	require.NoError(t, err)

	cert, err := crt.IssueCert(context.Background(), target, altNames, caName, "host")

	require.Error(t, err)
	require.Empty(t, cert)
	assert.True(t, issueCalled, "issue new cert not called")
}

func Test_crtClient_IssueCertDownloadErrStatus(t *testing.T) {
	downloadCalled := false
	downloadServer := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		downloadCalled = true
		w.WriteHeader(http.StatusInternalServerError)
		_, err := io.WriteString(w, fixtures.FullPem)
		require.NoError(t, err)
	}))
	defer downloadServer.Close()

	issueCalled := false
	issueServer := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		issueCalled = true
		w.WriteHeader(issueSuccessCode)
		_, err := io.WriteString(w, fmt.Sprintf(`{"download2":"%s"}`, downloadServer.URL))
		require.NoError(t, err)
	}))
	defer issueServer.Close()

	crt, err := New(&nop.Logger{}, Config{
		ClientConfig: configNoRetry,
		URL:          issueServer.URL,
		OAuth:        oauth,
	})
	require.NoError(t, err)

	cert, err := crt.IssueCert(context.Background(), target, altNames, caName, "host")

	require.Error(t, err)
	require.Empty(t, cert)
	assert.True(t, downloadCalled, "download not called")
	assert.True(t, issueCalled, "issue new cert not called")
}
