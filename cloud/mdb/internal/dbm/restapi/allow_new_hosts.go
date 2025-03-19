package restapi

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/dbm"
)

func (c *Client) AreNewHostsAllowed(ctx context.Context, dom0 string) (dbm.NewHostsInfo, error) {
	resp, err := c.request(ctx, http.MethodGet, "dom0", &dom0, nil, nil, opentracing.Tag{Key: "dom0", Value: dom0})
	if err != nil {
		return dbm.NewHostsInfo{}, err
	}
	return parseAllowNewHostsResponse(resp.Body)
}

func (c *Client) UpdateNewHostsAllowed(ctx context.Context, dom0 string, allowNewHosts bool) error {
	var setAllowNewHosts struct {
		AllowNewHosts bool `json:"allow_new_hosts"`
	}
	setAllowNewHosts.AllowNewHosts = allowNewHosts
	reqBody, err := json.Marshal(setAllowNewHosts)
	if err != nil {
		return err
	}
	_, err = c.request(ctx, http.MethodPut, "dom0/allow_new_hosts", &dom0, nil, reqBody, opentracing.Tag{Key: "dom0", Value: dom0})
	if err != nil {
		return err
	}
	return nil
}

func parseAllowNewHostsResponse(body io.Reader) (dbm.NewHostsInfo, error) {
	var data struct {
		FQDN          string `json:"fqdn"`
		LastUpdatedBy string `json:"allow_new_hosts_updated_by"`
		AllowNewHosts bool   `json:"allow_new_hosts"`
	}
	if err := json.NewDecoder(body).Decode(&data); err != nil {
		return dbm.NewHostsInfo{}, fmt.Errorf("malformed DBM response: %s", err)
	}
	if data.FQDN == "" {
		return dbm.NewHostsInfo{}, dbm.ErrMissing
	}

	return dbm.NewHostsInfo{
		SetBy:           data.LastUpdatedBy,
		NewHostsAllowed: data.AllowNewHosts,
	}, nil
}
