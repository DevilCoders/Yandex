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
package server

import (
	"bufio"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/common"
	"a.yandex-team.ru/library/go/core/log"
)

type agentHandler struct {
	agentID         string
	requestIDs      chan string
	pendingRequests sync.Map
	logger          log.Logger
}

func (a *agentHandler) handleRequest(w http.ResponseWriter, r *http.Request) {
	requestID := r.Header.Get(common.HeaderRequestID)
	if requestID == "" {
		a.handleListRequests(w, r)
		return
	}
	if r.Method == http.MethodPost {
		a.handlePostResponse(w, r, requestID)
		return
	}
	a.handleGetRequest(w, r, requestID)
}

func (a *agentHandler) handleListRequests(w http.ResponseWriter, r *http.Request) {
	requestIDs := a.waitForRequestIDs(r.Context())
	respJSON, err := json.Marshal(requestIDs)
	if err != nil {
		http.Error(w, fmt.Sprintf("Failure serializing the request IDs: %v", err), http.StatusInternalServerError)
		return
	}
	a.logger.Debugf("Reporting pending requests: %s", respJSON)
	w.WriteHeader(http.StatusOK)
	_, err = w.Write(respJSON)
	if err != nil {
		a.logger.Errorf("Failed to write response: %s", err)
	}
}

// waitForRequestIDs blocks until at least one request ID is available, and then returns
// a slice of all of the IDs available at that time.
//
// Note that any IDs returned by this method will never be returned again.
func (a *agentHandler) waitForRequestIDs(ctx context.Context) []string {
	var requestIDs []string
	select {
	case <-ctx.Done():
		return nil
	case <-time.After(30 * time.Second):
		return nil
	case id := <-a.requestIDs:
		requestIDs = append(requestIDs, id)
	}
	for {
		select {
		case id := <-a.requestIDs:
			requestIDs = append(requestIDs, id)
		default:
			return requestIDs
		}
	}
}

func (a *agentHandler) handleGetRequest(w http.ResponseWriter, r *http.Request, requestID string) {
	val, ok := a.pendingRequests.Load(requestID)
	if !ok {
		a.logger.Warnf("Could not find pending request: %q", requestID)
		http.NotFound(w, r)
		return
	}
	pending := val.(*pendingRequest)
	a.logger.Debugf("Returning pending request: %q", requestID)
	for key, value := range pending.serviceHeaders {
		w.Header().Set(key, value)
	}

	w.WriteHeader(http.StatusOK)
	err := pending.req.Write(w)
	if err != nil {
		a.logger.Errorf("Failed while sending user request to agent: %s", err)
		pending.errStatusChan <- http.StatusServiceUnavailable
	}
}

func (a *agentHandler) handlePostResponse(w http.ResponseWriter, r *http.Request, requestID string) {
	val, ok := a.pendingRequests.Load(requestID)
	if !ok {
		a.logger.Warnf("Could not find pending request: %q", requestID)
		http.NotFound(w, r)
		return
	}
	pending := val.(*pendingRequest)
	resp, err := http.ReadResponse(bufio.NewReader(r.Body), pending.req)
	if err != nil {
		a.logger.Errorf("Could not parse response to request %q: %s", requestID, err)
		http.Error(w, "Failure parsing request body", http.StatusBadRequest)
		return
	}
	// We want to track whether or not the body has finished being read so that we can
	// make sure that this method does not return until after that. However, we do not
	// want to block the sending of the response to the client while it is being read.
	//
	// To accommodate both goals, we replace the response body with a pipereader, start
	// forwarding the response immediately, and then copy the original body to the
	// corresponding pipewriter.
	respBody := resp.Body
	defer func() {
		err = respBody.Close()
		if err != nil {
			a.logger.Errorf("Failed to close response body: %s", err)
		}
	}()

	pr, pw := io.Pipe()
	defer func() {
		err = pw.Close()
		if err != nil {
			a.logger.Errorf("Failed to close pipe writer: %s", err)
		}
	}()

	resp.Body = pr
	select {
	case <-r.Context().Done():
		return
	case pending.respChan <- resp:
	}
	if _, err := io.Copy(pw, respBody); err != nil {
		a.logger.Errorf("Could not read response to request %q: %s", requestID, err)
		http.Error(w, "Failure reading request body", http.StatusInternalServerError)
	}
}
