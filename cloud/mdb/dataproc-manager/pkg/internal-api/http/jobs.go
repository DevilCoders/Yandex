package client

import (
	"bytes"
	"context"
	"fmt"
	"net/http"
	urlpkg "net/url"
	"strconv"

	"github.com/golang/protobuf/jsonpb"

	intapi "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/dataproc/v1"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *client) ListClusterJobs(ctx context.Context, in *intapi.ListJobsRequest) (*intapi.ListJobsResponse, error) {
	url := c.cfg.URL + "/mdb/hadoop/1.0/jobs"
	request, err := http.NewRequest(http.MethodGet, url, nil)
	if err != nil {
		return nil, xerrors.Errorf("failed to build request: %w", err)
	}

	q := request.URL.Query()
	if in.PageToken != "" {
		q.Add("pageToken", in.PageToken)
	}
	if in.PageSize > 0 {
		q.Add("pageSize", strconv.Itoa(int(in.PageSize)))
	}
	q.Add("filters", in.Filter)
	q.Add("cluster_id", in.ClusterId)
	request.URL.RawQuery = q.Encode()

	body, err := c.sendRequest(ctx, request)
	if err != nil {
		return nil, xerrors.Errorf("internal API request failed: %w", err)
	}

	var response intapi.ListJobsResponse
	unmarshaler := jsonpb.Unmarshaler{
		AllowUnknownFields: true,
	}
	err = unmarshaler.Unmarshal(bytes.NewBuffer(body), &response)
	if err != nil {
		return nil, xerrors.Errorf("failed to unmarshal ListJobsResponse from json: %w", err)
	}

	return &response, nil
}

func (c *client) UpdateJobStatus(ctx context.Context, in *intapi.UpdateJobStatusRequest) (*intapi.UpdateJobStatusResponse, error) {
	buffer := &bytes.Buffer{}
	marshaler := jsonpb.Marshaler{EnumsAsInts: false, OrigName: true}
	err := marshaler.Marshal(buffer, in)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal UpdateJobStatusRequest to json: %w", err)
	}

	url := fmt.Sprintf("%s/mdb/hadoop/1.0/clusters/%s/jobs/%s:updateStatus",
		c.cfg.URL, urlpkg.PathEscape(in.ClusterId), urlpkg.PathEscape(in.JobId))
	request, err := http.NewRequest(http.MethodPatch, url, buffer)
	if err != nil {
		return nil, xerrors.Errorf("failed to build request: %w", err)
	}
	request.Header.Set("Content-Type", "application/json")

	body, err := c.sendRequest(ctx, request)
	if err != nil {
		return nil, xerrors.Errorf("internal API request failed: %w", err)
	}

	var response intapi.UpdateJobStatusResponse
	unmarshaler := jsonpb.Unmarshaler{
		AllowUnknownFields: true,
	}
	err = unmarshaler.Unmarshal(bytes.NewBuffer(body), &response)
	if err != nil {
		return nil, xerrors.Errorf("failed to unmarshal UpdateJobStatusResponse from json: %w", err)
	}

	return &response, nil
}
