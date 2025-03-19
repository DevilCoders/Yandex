package restapi

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/url"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func parseVolumesResponse(body io.Reader) (map[string]dbm.ContainerVolumes, error) {
	var data []struct {
		SpaceLimit int    `json:"space_limit"`
		Fqdn       string `json:"container"`
	}
	if err := json.NewDecoder(body).Decode(&data); err != nil {
		return nil, fmt.Errorf("malformed DBM response: %s", err)
	}

	result := map[string]dbm.ContainerVolumes{}

	for _, d := range data {
		cv, ok := result[d.Fqdn]
		if !ok {
			cv = dbm.ContainerVolumes{
				FQDN:    d.Fqdn,
				Volumes: []dbm.Volume{},
			}
			result[d.Fqdn] = cv
		}
		cv.Volumes = append(cv.Volumes, dbm.Volume{SpaceLimit: d.SpaceLimit})
	}

	return result, nil
}

func (c *Client) volumes(ctx context.Context, query string, tags ...opentracing.Tag) (map[string]dbm.ContainerVolumes, error) {
	queryValues := url.Values{}
	queryValues.Set("query", fmt.Sprintf("%s;limit=%d", query, containersLimit))
	resp, err := c.request(ctx, http.MethodGet, "volumes", nil, &queryValues, nil, tags...)
	if err != nil {
		return nil, err
	}
	ret, err := parseVolumesResponse(resp.Body)
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

func (c *Client) VolumesByDom0(ctx context.Context, dom0 string) (map[string]dbm.ContainerVolumes, error) {
	// actual DBM treat `dom0` parameter as regexp
	// https://github.yandex-team.ru/mdb/db_maintenance/blob/bcca26184cc5fa555fe7c9843fbccc07b13cec45/src/query_conf/get_containers.sql#L18
	return c.volumes(ctx, "dom0="+matchStringPattern(dom0), opentracing.Tag{Key: "dom0", Value: dom0})
}
