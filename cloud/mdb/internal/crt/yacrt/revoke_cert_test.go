package yacrt

import (
	"context"
	"fmt"
	"io"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/core/log/nop"
)

func TestClient_RevokeByAPIURL(t *testing.T) {
	t.Run("revoke happy path", func(t *testing.T) {
		revokeCalled := false
		revokeServer := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, req *http.Request) {
			revokeCalled = true

			assert.Equal(t, "OAuth "+oauth.Unmask(), req.Header.Get("Authorization"), "invalid authorization header")

			w.WriteHeader(http.StatusOK)
			_, err := io.WriteString(w, `{}`)
			require.NoError(t, err)
		}))
		defer revokeServer.Close()

		c, err := New(&nop.Logger{}, Config{
			ClientConfig: configNoRetry,
			URL:          "not_real_url",
			OAuth:        oauth,
		})
		require.NoError(t, err)

		err = c.RevokeByAPIURL(context.Background(), revokeServer.URL)
		require.NoError(t, err)

		assert.True(t, revokeCalled, "revoke should be called")
	})
}

func TestClient_DontRevokeNotIssued(t *testing.T) {
	revokeCalled := false
	revokeServer := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		revokeCalled = true
		w.WriteHeader(http.StatusOK)
		_, err := io.WriteString(w, `{}`)
		require.NoError(t, err)
	}))
	defer revokeServer.Close()

	listCalled := false
	listServer := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, req *http.Request) {
		listCalled = true
		w.WriteHeader(http.StatusOK)
		_, err := io.WriteString(w, fmt.Sprintf(`{
								    "count": 1,
								    "page": 1,
								    "next": null,
								    "previous": null,
								    "results": [
								        {
								            "common_name": "test.com",
								            "added": "2019-05-29T10:38:54.565081+03:00",
								            "end_date": "2021-05-28T10:28:54+03:00",
								            "issued": "2019-05-29T10:38:54.837755+03:00",
								            "updated": "2019-05-29T10:40:12.817277+03:00",
								            "revoked": null,
								            "serial_number": "3FEAF008000200075098",
								            "priv_key_deleted_at": null,
								            "status": "revoked",
								            "download2": "https://second-cert.com",
								            "ca_name": "WrongCA",
								            "hosts": [
								                "test.com",
												"test1.com",
												"test2.com"
								            ],
											"url": "%s"
								        }
								    ]
								}`, revokeServer.URL))
		require.NoError(t, err)

		assert.Equal(t, http.MethodGet, req.Method)
	}))
	defer listServer.Close()

	crt, err := New(&nop.Logger{}, Config{
		ClientConfig: configNoRetry,
		URL:          listServer.URL,
		OAuth:        oauth,
	})
	require.NoError(t, err)

	revoked, err := crt.RevokeCertsByHostname(context.Background(), "test.com")

	require.NoError(t, err)
	assert.False(t, revokeCalled, "revoke called")
	assert.True(t, listCalled, "list certs not called")
	assert.Equal(t, 0, revoked)
}

func TestClient_DontRevokeRevoked(t *testing.T) {
	revokeCalled := false
	revokeServer := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		revokeCalled = true
		w.WriteHeader(http.StatusOK)
		_, err := io.WriteString(w, `{}`)
		require.NoError(t, err)
	}))
	defer revokeServer.Close()

	listCalled := false
	listServer := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, req *http.Request) {
		listCalled = true
		w.WriteHeader(http.StatusOK)
		_, err := io.WriteString(w, fmt.Sprintf(`{
								    "count": 1,
								    "page": 1,
								    "next": null,
								    "previous": null,
								    "results": [
								        {
								            "common_name": "test.com",
								            "added": "2019-05-29T10:38:54.565081+03:00",
								            "end_date": "2021-05-28T10:28:54+03:00",
								            "issued": "2019-05-29T10:38:54.837755+03:00",
								            "updated": "2019-05-29T10:40:12.817277+03:00",
								            "revoked": "2019-05-30T10:40:12.817277+03:00",
								            "serial_number": "3FEAF008000200075098",
								            "priv_key_deleted_at": null,
								            "status": "issued",
								            "download2": "https://second-cert.com",
								            "ca_name": "WrongCA",
								            "hosts": [
								                "test.com",
												"test1.com",
												"test2.com"
								            ],
											"url": "%s"
								        }
								    ]
								}`, revokeServer.URL))
		require.NoError(t, err)

		assert.Equal(t, http.MethodGet, req.Method)
	}))
	defer listServer.Close()

	crt, err := New(&nop.Logger{}, Config{
		ClientConfig: configNoRetry,
		URL:          listServer.URL,
		OAuth:        oauth,
	})
	require.NoError(t, err)

	revoked, err := crt.RevokeCertsByHostname(context.Background(), "test.com")

	require.NoError(t, err)
	assert.False(t, revokeCalled, "revoke called")
	assert.True(t, listCalled, "list certs not called")
	assert.Equal(t, 0, revoked)
}

func TestClient_RevokeOneCert(t *testing.T) {
	revokeCalled := false
	revokeServer := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		revokeCalled = true
		w.WriteHeader(http.StatusOK)
		_, err := io.WriteString(w, `{}`)
		require.NoError(t, err)
	}))
	defer revokeServer.Close()

	listCalled := false
	listServer := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, req *http.Request) {
		listCalled = true
		w.WriteHeader(http.StatusOK)
		_, err := io.WriteString(w, fmt.Sprintf(`{
								    "count": 1,
								    "page": 1,
								    "next": null,
								    "previous": null,
								    "results": [
								        {
								            "common_name": "test.com",
								            "added": "2019-05-31T16:30:30.646742+03:00",
								            "end_date": "2021-05-30T16:20:30+03:00",
								            "issued": "2019-05-31T16:30:30.903801+03:00",
								            "updated": "2019-05-31T16:35:02.611244+03:00",
								            "revoked": null,
								            "serial_number": "4B799D35000200075A56",
								            "priv_key_deleted_at": null,
								            "status": "issued",
								            "download2": "https://first-cert.com",
								            "ca_name": "InternalCA",
								            "hosts": [
								                "test.com"
								            ], 
											"url": "%s"
								        }
								    ]
								}`, revokeServer.URL))
		require.NoError(t, err)

		assert.Equal(t, http.MethodGet, req.Method)
	}))
	defer listServer.Close()

	crt, err := New(&nop.Logger{}, Config{
		ClientConfig: configNoRetry,
		URL:          listServer.URL,
		OAuth:        oauth,
	})
	require.NoError(t, err)

	revoked, err := crt.RevokeCertsByHostname(context.Background(), "test.com")

	require.NoError(t, err)
	assert.True(t, revokeCalled, "revoke not called")
	assert.True(t, listCalled, "list certs not called")
	assert.Equal(t, 1, revoked)
}
