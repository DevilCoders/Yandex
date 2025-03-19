package httpapi

import (
	"bytes"
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

type hostCreateGroup struct {
	ID int `json:"id"`
}

type hostCreateDatacenter struct {
	ID int `json:"id"`
}

type hostCreateHost struct {
	FQDN       string               `json:"fqdn"`
	ShortName  string               `json:"short_name"`
	Group      hostCreateGroup      `json:"group"`
	Datacenter hostCreateDatacenter `json:"datacenter"`
}

type hostCreateBody struct {
	Host hostCreateHost `json:"host"`
}

func (c *Client) HostCreate(ctx context.Context, request conductor.HostCreateRequest) error {
	url := fmt.Sprintf("%s/api/v1/hosts/", URL)

	rBody := hostCreateBody{Host: hostCreateHost{
		FQDN:      request.FQDN,
		ShortName: request.ShortName,
		Group: hostCreateGroup{
			ID: request.GroupID,
		},
		Datacenter: hostCreateDatacenter{
			ID: request.DataCenterID,
		},
	}}
	body, err := json.Marshal(&rBody)
	if err != nil {
		return err
	}

	req, err := http.NewRequest("POST", url, bytes.NewBuffer(body))
	if err != nil {
		return err
	}
	req.Header.Add("Content-Type", "application/json")

	req = req.WithContext(ctx)
	req.Header.Add("Authorization", tvm.FormatOAuthToken(c.cfg.OAuth.Unmask()))

	resp, err := c.httpClient.Do(req, "Conductor Create Host", tags.InstanceFQDN.Tag(request.FQDN))
	if err != nil {
		return xerrors.Errorf("error creating host: %w", err)
	}
	defer func() { _ = resp.Body.Close() }()

	if resp.StatusCode == http.StatusCreated {
		return nil
	}

	b, _ := ioutil.ReadAll(resp.Body)
	if resp.StatusCode == http.StatusUnprocessableEntity {
		return semerr.AlreadyExistsf("exists: %s", b)
	}
	return xerrors.Errorf("request failed with code %d: %s", resp.StatusCode, b)
}
