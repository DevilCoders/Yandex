package http

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/juggler/push"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	pushTimeout = 2 * time.Second
)

type Response struct {
	AcceptedEvents int             `json:"accepted_events"`
	Success        bool            `json:"success"`
	Events         []ResponseEvent `json:"events"`
	Message        string          `json:"message"`
}

type ResponseEvent struct {
	Code  int    `json:"code"`
	Error string `json:"error"`
}

func (p *Pusher) Push(ctx context.Context, request push.Request) error {
	data, err := json.Marshal(request)
	if err != nil {
		return xerrors.Errorf("marshal request: %w", err)
	}

	req, err := http.NewRequest("POST", p.url, bytes.NewReader(data))
	if err != nil {
		return xerrors.Errorf("new request: %w", err)
	}
	req.Header.Add("Content-Type", "application/json")
	reqCtx, cancel := context.WithTimeout(ctx, pushTimeout)
	defer cancel()
	req = req.WithContext(reqCtx)

	resp, err := p.client.Do(req, "push")
	if err != nil {
		return xerrors.Errorf("do push: %w", err)
	}

	defer func() { _ = resp.Body.Close() }()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return xerrors.Errorf("read response body: %w", err)
	}

	if resp.StatusCode != http.StatusOK {
		return xerrors.Errorf("push failed with code %d: %s", resp.StatusCode, body)
	}

	response := &Response{}
	if err = json.Unmarshal(body, response); err != nil {
		return xerrors.Errorf("unmarshal response %q: %w", body, err)
	}

	if !response.Success {
		return xerrors.Errorf("unsuccessful push: %s", response.Message)
	}

	if response.AcceptedEvents == len(request.Events) {
		return nil
	}

	if len(response.Events) != len(request.Events) {
		return xerrors.Errorf("response events count doesn't equal request events count, it's a juggler bug, request: %s, response: %s", data, body)
	}

	errors := make([]string, 0, len(request.Events)-response.AcceptedEvents)
	for i, event := range response.Events {
		if event.Code != http.StatusOK {
			errors = append(errors, fmt.Sprintf("can not send event %q because of error: %q", request.Events[i], event.Error))
		}
	}

	return xerrors.Errorf("some events were not sent: %s", strings.Join(errors, ", "))
}
