package testutil

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net"
	"net/http"
	"sync/atomic"
	"testing"

	"github.com/gorilla/websocket"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type WebsocketClient struct {
	httpClient     *http.Client
	baseURL        string
	sessionID      atomic.Value
	serverMessages chan typedMessage
	closedChan     chan struct{}
}

type wireMessage struct {
	SessionID string      `json:"id,omitempty"`
	Message   interface{} `json:"msg,omitempty"`
}

type typedMessage struct {
	Type int
	Data []byte
}

func NewWebsocketClient(ctx context.Context, t *testing.T, serverAddr net.Addr, httpClient *http.Client, shimPath, upstreamServerPath string) (*WebsocketClient, error) {
	baseURL := fmt.Sprintf("https://%s%s", serverAddr.String(), shimPath)
	requestBody := bytes.NewBufferString(upstreamServerPath)
	request, err := http.NewRequestWithContext(ctx, "POST", baseURL+"/open", requestBody)
	if err != nil {
		return nil, err
	}

	response, err := httpClient.Do(request)
	if err != nil {
		return nil, err
	}

	if response.StatusCode != 200 {
		body, err := ioutil.ReadAll(response.Body)
		if err != nil {
			return nil, xerrors.Errorf("failed to read response body: %s", err)
		}
		return nil, xerrors.Errorf("failed to open websocket connection: %s", body)
	}

	body, err := ioutil.ReadAll(response.Body)
	if err != nil {
		return nil, err
	}

	var message wireMessage
	err = json.Unmarshal(body, &message)
	if err != nil {
		return nil, err
	}

	we := &WebsocketClient{
		httpClient:     httpClient,
		baseURL:        baseURL,
		serverMessages: make(chan typedMessage, 10),
		closedChan:     make(chan struct{}),
	}
	we.SetSessionID(message.SessionID)
	go we.poll(ctx, t)
	return we, nil
}

func (client *WebsocketClient) poll(ctx context.Context, t *testing.T) {
	childCtx, cancel := context.WithCancel(ctx)
	go func() {
		select {
		case <-childCtx.Done():
		case <-client.closedChan:
		}
		cancel()
	}()

	for {
		msg := wireMessage{SessionID: client.GetSessionID()}
		body, err := json.Marshal(&msg)
		require.NoError(t, err)

		request, err := http.NewRequestWithContext(childCtx, "POST", client.baseURL+"/poll", bytes.NewReader(body))
		require.NoError(t, err)

		response, err := client.httpClient.Do(request)
		if client.closed() || childCtx.Err() != nil {
			return
		}
		require.NoError(t, err)

		if response.StatusCode == http.StatusGone {
			close(client.closedChan)
			return
		}

		if response.StatusCode != http.StatusOK {
			return
		}

		var messages []string
		body, err = ioutil.ReadAll(response.Body)
		require.NoError(t, err)

		err = json.Unmarshal(body, &messages)
		require.NoError(t, err)
		for _, msg := range messages {
			message := typedMessage{
				Type: websocket.TextMessage,
				Data: []byte(msg),
			}
			select {
			case <-ctx.Done():
				return
			case client.serverMessages <- message:
			}
		}
	}
}

func (client *WebsocketClient) ReadMessage(ctx context.Context) (string, error) {
	select {
	case <-ctx.Done():
		return "", ctx.Err()
	case message := <-client.serverMessages:
		return string(message.Data), nil
	}
}

func (client *WebsocketClient) WriteMessage(ctx context.Context, msg string) error {
	messages := []wireMessage{{
		SessionID: client.GetSessionID(),
		Message:   msg,
	}}
	body, err := json.Marshal(&messages)
	if err != nil {
		return err
	}

	request, err := http.NewRequestWithContext(ctx, "POST", client.baseURL+"/data", bytes.NewReader(body))
	if err != nil {
		return err
	}

	response, err := client.httpClient.Do(request)
	if err != nil {
		return err
	}

	if response.StatusCode != http.StatusOK {
		body, _ := ioutil.ReadAll(response.Body)
		return xerrors.Errorf("failed to send websocket message: %s", body)
	}
	return nil
}

func (client *WebsocketClient) closed() bool {
	select {
	case <-client.closedChan:
		return true
	default:
		return false
	}
}

func (client *WebsocketClient) Close(ctx context.Context) error {
	if client.closed() {
		return nil
	}
	close(client.closedChan)

	message := wireMessage{
		SessionID: client.GetSessionID(),
	}
	body, err := json.Marshal(&message)
	if err != nil {
		return err
	}

	request, err := http.NewRequestWithContext(ctx, "POST", client.baseURL+"/close", bytes.NewReader(body))
	if err != nil {
		return err
	}

	response, err := client.httpClient.Do(request)
	if err != nil {
		return err
	}

	if response.StatusCode != http.StatusOK {
		body, _ := ioutil.ReadAll(response.Body)
		return xerrors.Errorf("failed to close websocket connection: %s", body)
	}
	return nil
}

func (client *WebsocketClient) ClosedChan() <-chan struct{} {
	return client.closedChan
}

func (client *WebsocketClient) GetSessionID() string {
	return client.sessionID.Load().(string)
}

func (client *WebsocketClient) SetSessionID(sessionID string) {
	client.sessionID.Store(sessionID)
}
