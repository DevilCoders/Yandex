package http

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/tracker"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Config struct {
	Endpoint string                `json:"endpoint" yaml:"endpoint"`
	Client   httputil.ClientConfig `json:"client" yaml:"client"`
	Token    secret.String         `json:"token" yaml:"token"`
}

func DefaultConfig() Config {
	return Config{
		Client: httputil.ClientConfig{
			Name:      "MDB Tracker Client",
			Transport: httputil.DefaultTransportConfig(),
		},
		Endpoint: "https://st-api.yandex-team.ru",
	}
}

func New(cfg Config, l log.Logger) (*api, error) {
	client, err := httputil.NewClient(cfg.Client, l)
	if err != nil {
		return nil, err
	}
	return &api{
		cfg:    cfg,
		client: client,
	}, nil
}

type api struct {
	cfg    Config
	client *httputil.Client
}

var _ tracker.API = &api{}

func (a *api) newRequest(ctx context.Context, method, path string, data interface{}) (*http.Request, error) {
	requestBytes, err := json.Marshal(data)
	if err != nil {
		return nil, xerrors.Errorf("marshal request: %w", err)
	}
	url, err := httputil.JoinURL(a.cfg.Endpoint, path)
	if err != nil {
		return nil, xerrors.Errorf("join url: %w", err)
	}

	dataReader := bytes.NewReader(requestBytes)
	req, err := http.NewRequest(method, url, dataReader)
	if err != nil {
		return nil, xerrors.Errorf("build request: %w", err)
	}
	req.Header.Set("X-Request-Id", requestid.MustFromContext(ctx))
	req.Header.Set("Accept", "application/json")
	req.Header.Set("Content-Type", "application/json")
	req.Header.Add("Authorization", "OAuth "+a.cfg.Token.Unmask())
	return req, nil
}

func (a *api) do(ctx context.Context, req *http.Request, expectedCode int, opName string, tags ...opentracing.Tag) ([]byte, error) {
	requestID := requestid.MustFromContext(ctx)

	resp, err := a.client.Do(
		req.WithContext(ctx),
		opName,
		tags...,
	)
	if err != nil {
		return nil, xerrors.Errorf(
			"make request to URL %q, X-Request-Id: %s : %w",
			req.URL,
			requestid.MustFromContext(ctx),
			err,
		)
	}
	defer func() { _ = resp.Body.Close() }()

	respBody, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, xerrors.Errorf(
			"read response body (error code %d, X-Request-Id: %s): %w",
			resp.StatusCode,
			requestID,
			err,
		)
	}

	if resp.StatusCode != expectedCode {
		return nil, xerrors.Errorf(
			"API error code: %d, X-Request-Id: %s, body: %s",
			resp.StatusCode,
			requestID,
			respBody,
		)
	}
	return respBody, nil
}

func (a *api) CreateComment(ctx context.Context, issueID string, comment tracker.Comment) (string, error) {
	ctx = requestid.WithRequestID(ctx, requestid.New())
	// https://st-api.yandex-team.ru/docs/#operation/createCommentApiV2
	var createRequest struct {
		Text string `json:"text"`
	}
	createRequest.Text = comment.Text
	req, err := a.newRequest(ctx, http.MethodPost, fmt.Sprintf("v2/issues/%s/comments", issueID), createRequest)
	if err != nil {
		return "", xerrors.Errorf("form create comment request: %w", err)
	}

	respBody, err := a.do(
		ctx,
		req,
		http.StatusCreated,
		"Create Issue Comment",
		opentracing.Tag{Key: "issue.id", Value: issueID},
	)
	if err != nil {
		return "", err
	}
	var createResponse struct {
		LongID string `json:"longId"`
	}
	if err := json.Unmarshal(respBody, &createResponse); err != nil {
		return "", xerrors.Errorf("unmarshal response(%s): %w", respBody, err)
	}

	return createResponse.LongID, nil
}

func (a *api) UpdateComment(ctx context.Context, issueID, commentID string, comment tracker.Comment) error {
	ctx = requestid.WithRequestID(ctx, requestid.New())
	// https://st-api.yandex-team.ru/docs/#operation/updateCommentApiV2
	var updateRequest struct {
		Text string `json:"text"`
	}
	updateRequest.Text = comment.Text
	req, err := a.newRequest(ctx, http.MethodPatch, fmt.Sprintf("v2/issues/%s/comments/%s", issueID, commentID), updateRequest)
	if err != nil {
		return xerrors.Errorf("form update comment request: %w", err)
	}

	_, err = a.do(
		ctx,
		req,
		http.StatusOK,
		"Update Issue Comment",
		opentracing.Tag{Key: "issue.id", Value: issueID},
		opentracing.Tag{Key: "issue.comment.id", Value: commentID},
	)
	return err
}
