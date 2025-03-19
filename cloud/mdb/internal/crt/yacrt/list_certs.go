package yacrt

import (
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type listCertificatesResponse struct {
	Count   int
	Results []*certInfo
}

func (c *Client) listCertificates(ctx context.Context, hostname string) (listCertificatesResponse, error) {
	result := listCertificatesResponse{}
	request, err := http.NewRequest(http.MethodGet, c.url, nil)
	if err != nil {
		return result, err
	}
	request = request.WithContext(ctx)
	c.addAuthHeaderVal(request.Header)
	q := request.URL.Query()
	q.Add("host", hostname)
	request.URL.RawQuery = q.Encode()

	resp, err := c.cli.Do(request, "get_certificates")
	if err != nil {
		return result, err
	}
	defer func() { _ = resp.Body.Close() }()
	if resp.StatusCode != http.StatusOK {
		return result, xerrors.Errorf("certificator wrong response code: %d", resp.StatusCode)
	}
	respBody, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return result, err
	}
	err = json.Unmarshal(respBody, &result)
	return result, err
}
