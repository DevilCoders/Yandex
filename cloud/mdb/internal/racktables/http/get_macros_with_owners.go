package http

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/cloud/mdb/internal/racktables"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *Client) GetMacrosWithOwners(ctx context.Context) (racktables.MacrosWithOwners, error) {
	macrosWithOwners := racktables.MacrosWithOwners{}
	macrosWithOwnersResponse := racktables.GetMacrosWithOwnersResponse{}

	//url: https://ro.racktables.yandex-team.ru/export/network-macros-owners.php
	u := fmt.Sprintf("%s/export/network-macros-owners.php", c.config.Endpoint)

	resp, err := c.get(ctx, u, "Racktables get network macros owners")
	if err != nil {
		return macrosWithOwners, xerrors.Errorf("get response from racktables: %w", err)
	}
	defer func() { _ = resp.Body.Close() }()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return macrosWithOwners, xerrors.Errorf("read body (error code %d): %w", resp.StatusCode, err)
	}

	if resp.StatusCode != http.StatusOK {
		return macrosWithOwners, semerr.Internalf("racktables bad response with error code '%d' and body '%s'", resp.StatusCode, body)
	}

	if err = json.Unmarshal(body, &macrosWithOwnersResponse); err != nil {
		return macrosWithOwners, xerrors.Errorf("unmarshal json from response body: %w", err)
	}

	for macroName, macroOwners := range macrosWithOwnersResponse {
		macrosWithOwners[macroName] = macroOwners.Owners
	}

	return macrosWithOwners, nil
}
