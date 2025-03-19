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
	"bufio"
	"context"
	"net/http"
	"time"

	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/common"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type requestProcessor struct {
	agent *Agent

	requestID string
	user      string
	startTime time.Time
	request   *http.Request
}

func newRequestProcessor(agent *Agent, requestID string) *requestProcessor {
	return &requestProcessor{
		requestID: requestID,
		agent:     agent,
	}
}

// process reads a single request from the proxy server and forwards it to the upstream server.
func (rp *requestProcessor) process(ctx context.Context) {
	ctx = ctxlog.WithFields(ctx, log.String("RequestID", rp.requestID))
	err := rp.doProcess(ctx)
	if err != nil {
		ctxlog.Errorf(ctx, rp.agent.logger, "Failed to process request: %s", err)
	}
}

// process reads a single request from the proxy server and forwards it to the upstream server.
func (rp *requestProcessor) doProcess(ctx context.Context) error {
	proxyURL := rp.agent.config.ProxyServerURL + RequestPath
	proxyReq, err := http.NewRequestWithContext(ctx, http.MethodGet, proxyURL, nil)
	if err != nil {
		return err
	}
	proxyReq.Header.Add(common.HeaderAgentID, rp.agent.config.AgentID)
	proxyReq.Header.Add(common.HeaderRequestID, rp.requestID)
	proxyResp, err := rp.agent.httpClient.Do(proxyReq)
	if err != nil {
		return xerrors.Errorf("proxy request failed: %w", err)
	}
	defer func() {
		err = proxyResp.Body.Close()
		if err != nil {
			ctxlog.Errorf(ctx, rp.agent.logger, "Failed to close response body: %s", err)
		}
	}()

	if proxyResp.StatusCode != http.StatusOK {
		return xerrors.Errorf("error status while reading request from the proxy: %d", proxyResp.StatusCode)
	}

	err = rp.parseRequestFromProxyResponse(ctx, proxyResp)
	if err != nil {
		return xerrors.Errorf("failed to parse user request from proxy response: %w", err)
	}

	err = rp.forwardRequest(ctx, proxyResp.Header.Get(common.HeaderUpstreamHost))
	if err != nil {
		return xerrors.Errorf("failed to forward request: %w", err)
	}
	return nil
}

func (rp *requestProcessor) parseRequestFromProxyResponse(ctx context.Context, proxyResp *http.Response) error {
	rp.user = proxyResp.Header.Get(HeaderUserID)
	startTimeStr := proxyResp.Header.Get(common.HeaderRequestStartTime)

	startTime, err := time.Parse(time.RFC3339Nano, startTimeStr)
	if err != nil {
		return err
	}
	rp.startTime = startTime

	request, err := http.ReadRequest(bufio.NewReader(proxyResp.Body))
	if err != nil {
		return err
	}
	rp.request = request.WithContext(ctx)

	return nil
}

// forwardRequest forwards the given request from the proxy server to
// the upstream server and reports the response back to the proxy server.
func (rp *requestProcessor) forwardRequest(ctx context.Context, upstreamHost string) error {
	if rp.agent.config.ForwardUserID {
		rp.request.Header.Add(HeaderUserID, rp.user)
	}
	responseForwarder, err := NewResponseForwarder(ctx, rp)
	if err != nil {
		return xerrors.Errorf("failed to create response forwarder: %w", err)
	}

	handler, err := rp.agent.getUpstreamHandler(ctx, upstreamHost)
	if err != nil {
		return err
	}

	handler.ServeHTTP(responseForwarder, rp.request)
	ctxlog.Infof(ctx, rp.agent.logger, "Response %s received from upstream after %s",
		responseForwarder.response.Status, time.Since(rp.startTime).String())
	if err := responseForwarder.Close(); err != nil {
		return xerrors.Errorf("failed to close response forwarder: %w", err)
	}
	return nil
}
