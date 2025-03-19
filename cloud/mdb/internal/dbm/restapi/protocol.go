package restapi

import (
	"bytes"
	"context"
	"fmt"
	"io"
	"net/http"
	"net/url"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

func (c *Client) buildURL(collection string, resourceID *string, queryParams *url.Values) string {
	encodedQuery := ""
	if queryParams != nil {
		encodedQuery = queryParams.Encode()
		if encodedQuery != "" {
			encodedQuery = "?" + encodedQuery
		}
	}
	path := collection + "/"
	if resourceID != nil {
		path += url.PathEscape(*resourceID)
	}
	return fmt.Sprintf(
		"https://%s/api/v2/%s%s",
		c.host,
		path,
		encodedQuery,
	)
}

func (c *Client) possiblyWithBody(httpMethod string) bool {
	m := []string{http.MethodPost, http.MethodPut, http.MethodDelete, http.MethodPatch}
	return slices.ContainsString(m, httpMethod)
}

func (c *Client) request(ctx context.Context, httpMethod string, collection string, resourceID *string, query *url.Values, body []byte, tags ...opentracing.Tag) (*http.Response, error) {
	var bodyReader io.Reader = nil
	if body != nil {
		bodyReader = bytes.NewReader(body)
	}
	req, err := http.NewRequest(
		httpMethod,
		c.buildURL(collection, resourceID, query),
		bodyReader,
	)
	if err != nil {
		return nil, xerrors.Errorf("create request: %w", err)
	}
	req = req.WithContext(ctx)
	req.Header.Add("Accept", "application/json")
	if c.possiblyWithBody(httpMethod) {
		req.Header.Add("Content-Type", "application/json")
	}
	req.Header.Add("Authorization", "OAuth "+c.token)
	req.Header.Add("X-Request-Id", requestid.FromContextOrNew(ctx))

	var resp *http.Response
	var DoNetworkCall = func() error {
		resp, err = c.httpClient.Do(req, collection, tags...)

		if err != nil {
			// treat all transport and related errors as unavailable
			return dbm.ErrUnavailable.Wrap(err)
		}

		if resp.StatusCode != http.StatusOK {
			return errorFromDBMResponse(resp)
		}
		return nil
	}

	if httpMethod == http.MethodGet {
		err = retry.Retry(ctx, DoNetworkCall, retry.WithMaxRetries(3))
	} else {
		err = DoNetworkCall()
	}

	return resp, err
}

func errorFromDBMResponse(resp *http.Response) error {
	whyError := xerrors.Errorf(
		"%s (X-Request-Id: %s)",
		resp.Status,
		resp.Header.Get("X-Request-Id"),
	)
	switch {
	case resp.StatusCode >= http.StatusInternalServerError:
		return dbm.ErrUnavailable.Wrap(whyError)
	case resp.StatusCode >= http.StatusBadRequest:
		if resp.StatusCode == 404 {
			return dbm.ErrMissing.Wrap(whyError)
		}
		return dbm.ErrBadRequest.Wrap(whyError)
	}
	return xerrors.Errorf("DBM error: %w", whyError)
}
