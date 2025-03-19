package testutil

import (
	"context"
	"net/http"
	"strings"
	"testing"

	"github.com/gorilla/websocket"
	"github.com/stretchr/testify/require"
)

type WebsocketMessage struct {
	Type    int
	Content []byte
}

type WebsocketBackend struct {
	t                *testing.T
	ctx              context.Context
	IncomingMessages chan string
	OutgoingMessages chan WebsocketMessage
	ConnectionClosed chan struct{}
	ConnectionOpened chan struct{}
	connection       *websocket.Conn
	OnConnect        func(*http.Request)
}

func NewWebsocketBackend(t *testing.T) *WebsocketBackend {
	return &WebsocketBackend{
		t:                t,
		ctx:              context.Background(),
		IncomingMessages: make(chan string),
		OutgoingMessages: make(chan WebsocketMessage),
		ConnectionClosed: make(chan struct{}),
		ConnectionOpened: make(chan struct{}),
	}
}

func (backend *WebsocketBackend) WithContext(ctx context.Context) {
	backend.ctx = ctx
}

func (backend *WebsocketBackend) Close(sendCloseMsg bool) {
	if sendCloseMsg {
		backend.OutgoingMessages <- WebsocketMessage{
			Type:    websocket.CloseMessage,
			Content: websocket.FormatCloseMessage(websocket.CloseNormalClosure, ""),
		}
	}

	close(backend.OutgoingMessages)
}

func (backend *WebsocketBackend) Handler() http.HandlerFunc {
	upgrader := websocket.Upgrader{}
	return func(w http.ResponseWriter, r *http.Request) {
		c, err := upgrader.Upgrade(w, r, nil)
		require.NoError(backend.t, err)
		backend.connection = c
		close(backend.ConnectionOpened)

		if backend.OnConnect != nil {
			backend.OnConnect(r)
		}

		ctx, cancel := context.WithCancel(backend.ctx)

		closeConn := make(chan struct{}, 2)
		// reader goroutine (this name is used in comment below)
		go func() {
			defer func() {
				close(backend.IncomingMessages) // unblock any potential reader
				closeConn <- struct{}{}
			}()
			for {
				select {
				case <-ctx.Done():
					return
				default:
					_, msg, err := c.ReadMessage()
					if err != nil {
						_, ok := err.(*websocket.CloseError)
						if ok || strings.Contains(err.Error(), "use of closed network connection") {
							return // connection is closed. this is ok
						}
						require.NoError(backend.t, err)
					}
					backend.IncomingMessages <- string(msg)
				}
			}
		}()

		// writer goroutine (this name is used in comment below)
		go func() {
			defer func() {
				closeConn <- struct{}{}
			}()
			for {
				select {
				case <-ctx.Done():
					return
				case msg, ok := <-backend.OutgoingMessages:
					if !ok {
						return
					}
					err = c.WriteMessage(msg.Type, msg.Content)
					require.NoError(backend.t, err)
				}
			}
		}()

		// there are following scenarios:
		// 1. context passed to this function is done. Then writer goroutine will finish
		//    (note that reader goroutine can't be finished directly by context as it is blocked
		//    by ReadMessage call). Before exit writer goroutine sends to closeConn.
		//    Then we receive from closeConn below. Then we close connection. This allows ReadMessage
		//    within reader goroutine to exit with error; and reader goroutine finishes.
		// 2. connection is closed due to any reason. Reader goroutine exits and sends to closeConn;
		//    we receive from closeConn below; then we cancel ctx; this stops writer goroutine.
		<-closeConn
		cancel()
		err = c.Close()
		require.NoError(backend.t, err)
		close(backend.ConnectionClosed)
	}
}
