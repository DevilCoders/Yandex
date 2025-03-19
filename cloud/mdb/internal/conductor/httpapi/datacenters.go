package httpapi

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/cloud/mdb/internal/auth/tvm"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type dataCenterResponse struct {
	ID int `json:"id" yaml:"id"`
}

func (c *Client) DCByName(ctx context.Context, name string) (conductor.DataCenter, error) {
	result := conductor.DataCenter{}
	url := fmt.Sprintf("%s/api/v1/datacenters/%s", URL, name)

	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return result, err
	}

	req = req.WithContext(ctx)
	req.Header.Add("Authorization", tvm.FormatOAuthToken(c.cfg.OAuth.Unmask()))

	resp, err := c.httpClient.Do(req, "Conductor Get DC", tags.DataCenter.Tag(name))
	if err != nil {
		return result, err
	}
	defer func() { _ = resp.Body.Close() }()

	b, _ := ioutil.ReadAll(resp.Body)
	if resp.StatusCode == http.StatusNotFound {
		return result, semerr.NotFoundf("datacenter %q not found", name)
	} else if resp.StatusCode != http.StatusOK {
		return result, xerrors.Errorf("request failed with code %d: %s", resp.StatusCode, b)
	}

	var dR dataCenterResponse
	err = json.Unmarshal(b, &dR)
	if err != nil {
		return result, fmt.Errorf("json response %s: %w", b, err)
	}
	result.ID = dR.ID
	return result, nil
}
