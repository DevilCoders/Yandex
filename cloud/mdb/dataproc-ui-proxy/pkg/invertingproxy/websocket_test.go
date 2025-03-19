package invertingproxy

import (
	"net/http"
	"testing"

	"github.com/gorilla/websocket"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/testutil"
)

func TestWebsocketBasic(t *testing.T) {
	backend := testutil.NewWebsocketBackend(t)
	control := setupTest(t, backend.Handler())
	backend.WithContext(control.ctx)

	client, err := testutil.NewWebsocketClient(control.ctx, t, control.proxyServerAddr,
		testProxyHTTPClient, testWebsocketShimPath, "/path/on/websocket/server")
	require.NoError(t, err)

	control.wait(backend.ConnectionOpened)

	err = client.WriteMessage(control.ctx, "who are you?")
	require.NoError(t, err)

	require.Equal(t, "who are you?", control.waitString(backend.IncomingMessages))
	backend.OutgoingMessages <- testutil.WebsocketMessage{
		Type:    websocket.TextMessage,
		Content: []byte("i'm websocket server"),
	}

	msg, err := client.ReadMessage(control.ctx)
	require.NoError(t, err)
	require.Equal(t, "i'm websocket server", msg)

	err = client.Close(control.ctx)
	require.NoError(t, err)

	control.wait(backend.ConnectionClosed)
}

func TestNonWebsocketBackend(t *testing.T) {
	handler := func(w http.ResponseWriter, r *http.Request) {
		_, _ = w.Write([]byte("ok"))
	}
	control := setupTest(t, handler)

	_, err := testutil.NewWebsocketClient(control.ctx, t, control.proxyServerAddr,
		testProxyHTTPClient, testWebsocketShimPath, "/path/on/websocket/server")
	require.Error(t, err)
	require.Contains(t, err.Error(), "bad handshake")
}

func TestWebsocketBackendIsNotRunning(t *testing.T) {
	backend := testutil.NewWebsocketBackend(t)
	control := setupTest(t, backend.Handler())
	control.backendServer.Close()

	_, err := testutil.NewWebsocketClient(control.ctx, t, control.proxyServerAddr,
		testProxyHTTPClient, testWebsocketShimPath, "/path/on/websocket/server")
	require.Error(t, err)
	require.Contains(t, err.Error(), "connection refused")
}

func TestCloseWebsocketOnServer(t *testing.T) {
	backend := testutil.NewWebsocketBackend(t)
	control := setupTest(t, backend.Handler())
	backend.WithContext(control.ctx)

	client, err := testutil.NewWebsocketClient(control.ctx, t, control.proxyServerAddr,
		testProxyHTTPClient, testWebsocketShimPath, "/path/on/websocket/server")
	require.NoError(t, err)

	backend.Close(false)
	control.wait(client.ClosedChan())
}

func TestCloseWebsocketOnServerWithMessage(t *testing.T) {
	backend := testutil.NewWebsocketBackend(t)
	control := setupTest(t, backend.Handler())
	backend.WithContext(control.ctx)

	client, err := testutil.NewWebsocketClient(control.ctx, t, control.proxyServerAddr,
		testProxyHTTPClient, testWebsocketShimPath, "/path/on/websocket/server")
	require.NoError(t, err)

	backend.Close(true)
	control.wait(client.ClosedChan())
}

func TestWebsocketPath(t *testing.T) {
	backend := testutil.NewWebsocketBackend(t)
	control := setupTest(t, backend.Handler())
	backend.WithContext(control.ctx)

	backend.OnConnect = func(request *http.Request) {
		require.Equal(t, "/path/on/websocket/server", request.URL.Path)
	}

	_, err := testutil.NewWebsocketClient(control.ctx, t, control.proxyServerAddr,
		testProxyHTTPClient, testWebsocketShimPath, "/path/on/websocket/server")
	require.NoError(t, err)
}

func TestInvalidWebsocketSessionID(t *testing.T) {
	backend := testutil.NewWebsocketBackend(t)
	control := setupTest(t, backend.Handler())
	backend.WithContext(control.ctx)

	client, err := testutil.NewWebsocketClient(control.ctx, t, control.proxyServerAddr,
		testProxyHTTPClient, testWebsocketShimPath, "/path/on/websocket/server")
	require.NoError(t, err)
	client.SetSessionID("xxx")

	err = client.WriteMessage(control.ctx, "hello")
	require.Error(t, err)
	require.Contains(t, err.Error(), "unknown shim session ID")
}
