/*
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
// initially taken from https://github.com/google/inverting-proxy
package websockets

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/http/httputil"
	"net/url"
	"path"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"a.yandex-team.ru/library/go/core/log"
)

const (
	inactiveConnectionLifetime = time.Minute
	connectionCleanInterval    = 10 * time.Second
)

type sessionMessage struct {
	ID      string      `json:"id,omitempty"`
	Message interface{} `json:"msg,omitempty"`
}

// provides access to websocket connection and messages via http rest api
type Adapter struct {
	ctx          context.Context
	logger       log.Logger
	handler      http.Handler
	upstreamHost string
	connections  sync.Map
	sessionCount uint64
}

// NewAdapter creates a reverse proxy that inserts websocket-shim code into all HTML responses.
func NewAdapter(wrapped *httputil.ReverseProxy, upstreamHost, shimPath string, logger log.Logger) (*Adapter, error) {
	shimPath = strings.TrimLeft(shimPath, "/")
	var templateBuf bytes.Buffer
	if err := shimTmpl.Execute(&templateBuf, &struct{ ShimPath string }{ShimPath: shimPath}); err != nil {
		return nil, err
	}
	shimCode := templateBuf.String()
	wrapped.ModifyResponse = func(resp *http.Response) error {
		return shimBody(resp, shimCode)
	}
	mux := http.NewServeMux()
	shimPath = path.Clean("/"+shimPath) + "/"

	a := &Adapter{
		ctx:          context.Background(),
		logger:       logger,
		upstreamHost: upstreamHost,
	}
	mux.HandleFunc(path.Join(shimPath, "open"), a.openHandler)
	mux.HandleFunc(path.Join(shimPath, "close"), a.closeHandler)
	mux.HandleFunc(path.Join(shimPath, "data"), a.dataHandler)
	mux.HandleFunc(path.Join(shimPath, "poll"), a.pollHandler)

	mux.Handle("/", wrapped)
	a.handler = mux

	return a, nil
}

func (a *Adapter) Handler() http.Handler {
	return a.handler
}

func (a *Adapter) Run(ctx context.Context) {
	a.ctx = ctx
	a.runConnectionCleaner(ctx)
}

func (a *Adapter) runConnectionCleaner(ctx context.Context) {
	go func() {
		ticker := time.NewTicker(connectionCleanInterval)
		defer ticker.Stop()
		for {
			select {
			case <-ctx.Done():
				return
			case <-ticker.C:
				a.closeInactiveConnections()
			}
		}
	}()
}

func (a *Adapter) closeInactiveConnections() {
	a.connections.Range(func(key, value interface{}) bool {
		conn := value.(*Connection)
		if conn.LastUsedAt().Add(inactiveConnectionLifetime).Before(time.Now()) {
			a.logger.Infof("Closing websocket connection for session %q"+
				" due to long inactivity", key.(string))
			a.connections.Delete(key)
			conn.Close()
		}
		return true
	})
}

func (a *Adapter) openHandler(w http.ResponseWriter, r *http.Request) {
	sessionID := fmt.Sprintf("%d", atomic.AddUint64(&a.sessionCount, 1))
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		http.Error(w, fmt.Sprintf("internal error reading a shim request: %v", err), http.StatusInternalServerError)
		return
	}
	targetURL, err := url.Parse(string(body))
	if err != nil {
		http.Error(w, fmt.Sprintf("malformed shim open request: %v", err), http.StatusBadRequest)
		return
	}
	targetURL.Scheme = "ws"
	targetURL.Host = a.upstreamHost
	// (maybe) temporarily remove Origin header in order to overcome same origin protection
	// within websocket server. Some websocket servers check that Origin == Host if Origin is set.
	// At least chat example from gorilla does so.
	r.Header.Del("Origin")
	conn, err := NewConnection(a.ctx, targetURL.String(), r.Header, a.logger)
	if err != nil {
		message := fmt.Sprintf("Failed to open websocket connection to %q: %s", targetURL, err)
		a.logger.Error(message)
		http.Error(w, message, http.StatusBadGateway)
		return
	}
	a.connections.Store(sessionID, conn)
	a.logger.Debugf("Websocket connection to the server %q established for session: %v", targetURL, sessionID)
	resp := &sessionMessage{
		ID:      sessionID,
		Message: targetURL.String(),
	}
	respBytes, err := json.Marshal(resp)
	if err != nil {
		a.logger.Errorf("Failed to serialize the response to a websocket open request: %s", err)
		http.Error(w, fmt.Sprintf("internal error opening a shim connection: %v", err), http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	_, err = w.Write(respBytes)
	if err != nil {
		a.logger.Errorf("Failed to write response body: %s", err)
	}
}

func (a *Adapter) closeHandler(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		http.Error(w, fmt.Sprintf("internal error reading a shim request: %v", err), http.StatusInternalServerError)
		return
	}
	var msg sessionMessage
	if err := json.Unmarshal(body, &msg); err != nil {
		http.Error(w, fmt.Sprintf("error parsing a shim request: %v", err), http.StatusBadRequest)
		return
	}
	c, ok := a.connections.Load(msg.ID)
	if !ok {
		http.Error(w, fmt.Sprintf("unknown shim session ID: %q", msg.ID), http.StatusNotFound)
		return
	}
	conn := c.(*Connection)
	a.connections.Delete(msg.ID)
	conn.Close()
	w.Header().Set("Content-Type", "text/plain")
	w.WriteHeader(http.StatusOK)
	_, err = w.Write([]byte("ok"))
	if err != nil {
		a.logger.Errorf("Failed to write response body: %s", err)
	}
}

func (a *Adapter) dataHandler(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		http.Error(w, fmt.Sprintf("internal error reading a shim request: %v", err), http.StatusInternalServerError)
		return
	}
	var msgs []sessionMessage
	if err := json.Unmarshal(body, &msgs); err != nil {
		http.Error(w, fmt.Sprintf("error parsing a shim request: %v", err), http.StatusBadRequest)
		return
	}
	for _, msg := range msgs {
		c, ok := a.connections.Load(msg.ID)
		if !ok {
			http.Error(w, fmt.Sprintf("unknown shim session ID: %q", msg.ID), http.StatusNotFound)
			return
		}
		conn := c.(*Connection)
		if err := conn.SendClientMessage(msg.Message); err != nil {
			errMsg := fmt.Sprintf("failed to send message on session %q: %s", msg.ID, err)
			if err == ErrClosed {
				a.connections.Delete(msg.ID)
				http.Error(w, errMsg, http.StatusGone)
			} else {
				http.Error(w, errMsg, http.StatusBadRequest)
			}
			return
		}
	}
	w.Header().Set("Content-Type", "text/plain")
	w.WriteHeader(http.StatusOK)
	_, err = w.Write([]byte("ok"))
	if err != nil {
		a.logger.Errorf("Failed to write response body: %s", err)
	}
}

func (a *Adapter) pollHandler(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		http.Error(w, fmt.Sprintf("internal error reading a shim request: %v", err), http.StatusInternalServerError)
		return
	}
	var msg sessionMessage
	if err := json.Unmarshal(body, &msg); err != nil {
		http.Error(w, fmt.Sprintf("error parsing a shim request: %v", err), http.StatusBadRequest)
		return
	}
	c, ok := a.connections.Load(msg.ID)
	if !ok {
		http.Error(w, fmt.Sprintf("unknown shim session ID: %q", msg.ID), http.StatusNotFound)
		return
	}
	conn := c.(*Connection)
	serverMsgs, err := conn.ReadServerMessages()
	if err != nil {
		if err == ErrClosed {
			a.connections.Delete(msg.ID)
			http.Error(w, err.Error(), http.StatusGone)
		} else {
			a.logger.Errorf("Failed to read server messages: %s", err)
			status := http.StatusInternalServerError
			http.Error(w, http.StatusText(status), status)
		}
		return
	} else if serverMsgs == nil {
		w.WriteHeader(http.StatusRequestTimeout)
		return
	}
	respBytes, err := json.Marshal(serverMsgs)
	if err != nil {
		http.Error(w, fmt.Sprintf("internal error serializing the server-side messages %v", err), http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	_, err = w.Write(respBytes)
	if err != nil {
		a.logger.Errorf("Failed to write response body: %s", err)
	}
}
