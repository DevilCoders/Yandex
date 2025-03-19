package http

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/url"

	"a.yandex-team.ru/cloud/mdb/internal/racktables"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *Client) GetMacro(ctx context.Context, name string) (racktables.Macro, error) {
	macro := racktables.Macro{}

	//url: https://ro.racktables.yandex-team.ru/export/project-id-networks.php?op=show_macro&name=_PGAASINTERNALNETS_
	u := fmt.Sprintf("%s/export/project-id-networks.php?op=show_macro&name=%s", c.config.Endpoint, url.QueryEscape(name))

	resp, err := c.get(ctx, u, "Racktables Get Macro", tags.NetworkID.Tag(name))
	if err != nil {
		return macro, xerrors.Errorf("get response from racktables: %w", err)
	}
	defer func() { _ = resp.Body.Close() }()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return macro, xerrors.Errorf("read body (error code %d): %w", resp.StatusCode, err)
	}

	if resp.StatusCode != http.StatusOK {
		if resp.StatusCode == http.StatusInternalServerError && string(body) == fmt.Sprintf("Record 'macro'#'%s' does not exist", name) {
			return macro, semerr.NotFoundf("macro %s not found", name)
		}

		return macro, semerr.Internalf("racktables bad response with error code '%d' and body '%s'", resp.StatusCode, body)
	}

	if err = json.Unmarshal(body, &macro); err != nil {
		return macro, xerrors.Errorf("unmarshal json from response body: %w", err)
	}

	return macro, nil
}
