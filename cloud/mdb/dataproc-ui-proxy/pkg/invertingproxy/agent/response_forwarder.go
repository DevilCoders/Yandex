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
package agent

import (
	"context"
	"fmt"
	"io"
	"net/http"

	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/common"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// ResponseForwarder implements http.ResponseWriter by dumping a wire-compatible
// representation of the response to 'proxyWriter' field.
//
// ResponseForwarder is used by the agent to forward a response from the upstream
// server to the inverting proxy server.
type ResponseForwarder struct {
	proxyWriter        *io.PipeWriter
	startedChan        chan struct{}
	responseBodyWriter *io.PipeWriter
	logger             log.Logger

	// wroteHeader is set when WriteHeader is called. It's used to ensure a
	// call to WriteHeader before the first call to Write.
	wroteHeader bool

	// response is synthesized using the upstream server response. We use its Write
	// method as a convenience when forwarding the wire-representation received
	// by the upstream server.
	response *http.Response

	// header is used to store the response headers prior to sending them.
	// This is separate from the headers in the response as it includes hop headers,
	// which will be filtered out before sending the response.
	header http.Header

	// proxyClientErrors is a channel where any errors issuing a client request to
	// the proxy server get written.
	//
	// This is eventually returned to the caller of the Close method.
	proxyClientErrors chan error

	// forwardingErrors is a channel where all errors forwarding the streamed
	// response from the upstream server to the proxy server get written.
	//
	// This is eventually returned to the caller of the Close method.
	forwardingErrors chan error

	// writeErrors is a channel where all errors writing the streamed response
	// from the upstream server get written.
	//
	// This is eventually returned to the caller of the Close method.
	writeErrors chan error
}

// NewResponseForwarder constructs a new ResponseForwarder that forwards to the
// given proxy for the specified request.
func NewResponseForwarder(ctx context.Context, rp *requestProcessor) (*ResponseForwarder, error) {
	// The contortions below support streaming.
	//
	// There are two pipes:
	// 1. proxyReader, proxyWriter: The io.PipeWriter for the HTTP POST to the inverting proxy server.
	//       To this pipe, we write the full HTTP response from the upstream server in HTTP
	//       wire-format form. (Status + Headers + Body + Trailers)
	//
	// 2. responseBodyReader, responseBodyWriter: This pipe corresponds to the response body
	//       from the upstream server. To this pipe, we stream each read from upstream server.
	proxyReader, proxyWriter := io.Pipe()
	startedChan := make(chan struct{}, 1)
	responseBodyReader, responseBodyWriter := io.Pipe()

	proxyURL := rp.agent.config.ProxyServerURL + ResponsePath
	proxyReq, err := http.NewRequestWithContext(ctx, http.MethodPost, proxyURL, proxyReader)
	if err != nil {
		return nil, err
	}
	proxyReq.Header.Set(common.HeaderAgentID, rp.agent.config.AgentID)
	proxyReq.Header.Set(common.HeaderRequestID, rp.requestID)
	proxyReq.Header.Set("Content-Type", "text/plain")

	proxyClientErrChan := make(chan error, 100)
	forwardingErrChan := make(chan error, 100)
	writeErrChan := make(chan error, 100)
	go func() {
		// Wait until the response body has started being written
		// (for a non-empty response) or for the response to
		// be closed (for an empty response) before triggering
		// the proxy request round trip.
		//
		// This ensures that we do not fetch the bearer token
		// for the auth header until the last possible moment.
		// That, in turn. prevents a race condition where the
		// token expires between the header being generated
		// and the request being sent to the proxy.
		<-startedChan
		if _, err := rp.agent.httpClient.Do(proxyReq); err != nil {
			proxyClientErrChan <- err
		}
		close(proxyClientErrChan)
	}()

	return &ResponseForwarder{
		response: &http.Response{
			Proto:      "HTTP/1.1",
			ProtoMajor: 1,
			ProtoMinor: 1,
			Header:     make(http.Header),
			Body:       responseBodyReader,
		},
		wroteHeader:        false,
		header:             make(http.Header),
		proxyWriter:        proxyWriter,
		startedChan:        startedChan,
		responseBodyWriter: responseBodyWriter,
		proxyClientErrors:  proxyClientErrChan,
		forwardingErrors:   forwardingErrChan,
		writeErrors:        writeErrChan,
		logger:             rp.agent.logger,
	}, nil
}

func (rf *ResponseForwarder) notify() {
	if rf.startedChan != nil {
		rf.startedChan <- struct{}{}
		rf.startedChan = nil
	}
}

// Header implements the http.ResponseWriter interface.
func (rf *ResponseForwarder) Header() http.Header {
	return rf.header
}

func (rf *ResponseForwarder) Response() *http.Response {
	return rf.response
}

// Write implements the http.ResponseWriter interface.
func (rf *ResponseForwarder) Write(buf []byte) (int, error) {
	// As in net/http, call WriteHeader if it has not yet been called
	// before the first call to Write.
	if !rf.wroteHeader {
		rf.WriteHeader(http.StatusOK)
	}
	rf.notify()
	count, err := rf.responseBodyWriter.Write(buf)
	if err != nil {
		rf.writeErrors <- err
	}
	return count, err
}

// WriteHeader implements the http.ResponseWriter interface.
func (rf *ResponseForwarder) WriteHeader(code int) {
	// As in net/http, ignore multiple calls to WriteHeader.
	if rf.wroteHeader {
		return
	}
	rf.wroteHeader = true
	for _, h := range common.HopHeaders {
		rf.header.Del(h)
	}
	for k, v := range rf.header {
		rf.response.Header[k] = v
	}
	rf.response.StatusCode = code
	rf.response.Status = fmt.Sprintf("%d %s", code, http.StatusText(code))
	// This will write the status and headers immediately and stream the
	// body using the pipes we've wired.
	go func() {
		defer func() {
			err := rf.proxyWriter.Close()
			if err != nil {
				rf.logger.Errorf("Failed to close pipe writer: %s", err)
			}
		}()
		if err := rf.response.Write(rf.proxyWriter); err != nil {
			rf.forwardingErrors <- err

			// Normally, the end of this goroutine indicates
			// that the response.Body reader has returned an EOF,
			// which means that the corresponding writer has been
			// closed. However, that is not necessarily the case
			// if we hit an error in the call to `Write`.
			//
			// In this case, there may still be someone writing
			// to the pipe writer, but we will no longer be reading
			// anything from the corresponding reader. As such,
			// we signal that issue to any remaining writers.
			err = rf.response.Body.(*io.PipeReader).CloseWithError(err)
			if err != nil {
				rf.logger.Errorf("Failed to close response body: %s", err)
			}
		}
		close(rf.forwardingErrors)
	}()
}

// Close signals that the response has been fully read from the upstream server,
// waits for that response to be forwarded to the proxy, and then reports any
// errors that occured while forwarding the response.
func (rf *ResponseForwarder) Close() error {
	rf.notify()
	var errs []error
	if err := rf.responseBodyWriter.Close(); err != nil {
		errs = append(errs, err)
	}
	for err := range rf.proxyClientErrors {
		errs = append(errs, err)
	}
	for err := range rf.forwardingErrors {
		errs = append(errs, err)
	}
	close(rf.writeErrors)
	for err := range rf.writeErrors {
		errs = append(errs, err)
	}
	if len(errs) > 0 {
		return xerrors.Errorf("multiple errors closing pipe writers: %s", errs)
	}
	return nil
}
