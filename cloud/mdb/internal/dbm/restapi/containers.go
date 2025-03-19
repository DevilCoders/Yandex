package restapi

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"regexp"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const containersLimit = 100500

func parseContainersResponse(body io.Reader) ([]dbm.Container, error) {
	var data []struct {
		ClusterName string `json:"cluster_name"`
		FQDN        string `json:"fqdn"`
		Dom0        string `json:"dom0"`
	}
	if err := json.NewDecoder(body).Decode(&data); err != nil {
		return nil, fmt.Errorf("malformed DBM response: %s", err)
	}

	ret := make([]dbm.Container, len(data))

	for i, d := range data {
		ret[i] = dbm.Container{
			ClusterName: d.ClusterName,
			FQDN:        d.FQDN,
			Dom0:        d.Dom0,
		}
	}
	return ret, nil
}

func (c *Client) containers(ctx context.Context, query string, tags ...opentracing.Tag) ([]dbm.Container, error) {
	queryValues := url.Values{}
	queryValues.Set("query", fmt.Sprintf("%s;limit=%d", query, containersLimit))
	resp, err := c.request(ctx, http.MethodGet, "containers", nil, &queryValues, nil, tags...)
	if err != nil {
		return nil, err
	}
	ret, err := parseContainersResponse(resp.Body)
	if err != nil {
		return nil, err
	}
	if len(ret) == containersLimit {
		return nil,
			xerrors.Errorf(
				"this was unlikely, but it happened - we got %d containers from the DBM."+
					" It's not supported by that code."+
					" Looks like we need to write the logic of selecting all pages",
				containersLimit,
			)
	}

	return ret, nil
}

// MatchStringPattern create regex pattern that match only given string
func matchStringPattern(s string) string {
	return "^" + regexp.QuoteMeta(s) + "$"
}

func (c *Client) Dom0Containers(ctx context.Context, dom0 string) ([]dbm.Container, error) {
	// actual DBM treat `dom0` parameter as regexp
	// https://github.yandex-team.ru/mdb/db_maintenance/blob/bcca26184cc5fa555fe7c9843fbccc07b13cec45/src/query_conf/get_containers.sql#L18
	return c.containers(ctx, "dom0="+matchStringPattern(dom0), opentracing.Tag{Key: "dom0", Value: dom0})
}

func (c *Client) ClusterContainers(ctx context.Context, clusterName string) ([]dbm.Container, error) {
	// actual DBM treat `cluster_name` parameter as regexp
	// https://github.yandex-team.ru/mdb/db_maintenance/blob/bcca26184cc5fa555fe7c9843fbccc07b13cec45/src/query_conf/get_containers.sql#L19
	return c.containers(ctx, "cluster_name="+matchStringPattern(clusterName), opentracing.Tag{Key: "cluster_name", Value: clusterName})
}
