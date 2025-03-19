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

type v2GroupAttributes struct {
	Name string `json:"name"`
}

type v2Links struct {
	Related string `json:"related"`
}

type v2Relationship struct {
	Links v2Links `json:"links"`
}

type v2GroupRelationships struct {
	Parents v2Relationship `json:"parents"`
}

type v2GroupDefinition struct {
	Attributes    v2GroupAttributes    `json:"attributes"`
	Relationships v2GroupRelationships `json:"relationships"`
}

type v2GroupResponse struct {
	Value v2GroupDefinition `json:"data"`
}

type v2GroupParentResponse struct {
	Value []v2GroupDefinition `json:"data"`
}

func (c *Client) internalGroupToGroupInfo(gd v2GroupDefinition) conductor.GroupInfo {
	return conductor.GroupInfo{
		Name:      gd.Attributes.Name,
		URLParent: gd.Relationships.Parents.Links.Related,
	}
}

func (c *Client) GroupInfoByName(ctx context.Context, group string) (conductor.GroupInfo, error) {
	result := conductor.GroupInfo{}
	url := fmt.Sprintf("%s/api/v2/groups/%s", URL, group)

	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return result, err
	}

	req = req.WithContext(ctx)
	req.Header.Add("Authorization", tvm.FormatOAuthToken(c.cfg.OAuth.Unmask()))

	resp, err := c.httpClient.Do(req, "Conductor Get Group Info", tags.ConductorGroup.Tag(group))
	if err != nil {
		return result, err
	}
	defer func() { _ = resp.Body.Close() }()

	if resp.StatusCode == http.StatusNotFound {
		return result, semerr.NotFoundf("group %q not found", group)
	} else if resp.StatusCode != http.StatusOK {
		b, _ := ioutil.ReadAll(resp.Body)
		return result, xerrors.Errorf("request failed with code %d: %s", resp.StatusCode, b)
	}

	var gR v2GroupResponse
	err = json.NewDecoder(resp.Body).Decode(&gR)
	if err != nil {
		return result, fmt.Errorf("json response: %w", err)
	}
	return c.internalGroupToGroupInfo(gR.Value), nil
}

func (c *Client) ParentGroup(ctx context.Context, groupInfo conductor.GroupInfo) (conductor.GroupInfo, error) {
	result := conductor.GroupInfo{}
	url := groupInfo.URLParent

	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return result, err
	}

	req = req.WithContext(ctx)
	req.Header.Add("Authorization", tvm.FormatOAuthToken(c.cfg.OAuth.Unmask()))

	resp, err := c.httpClient.Do(req, "Conductor Get Parent Group", tags.ConductorGroup.Tag(groupInfo.Name))
	if err != nil {
		return result, err
	}
	defer func() { _ = resp.Body.Close() }()

	if resp.StatusCode == http.StatusNotFound {
		return result, semerr.NotFoundf("group '%s' not found", groupInfo.Name)
	} else if resp.StatusCode != http.StatusOK {
		b, _ := ioutil.ReadAll(resp.Body)
		return result, xerrors.Errorf("request code %d: %s", resp.StatusCode, b)
	}

	var gR v2GroupParentResponse
	err = json.NewDecoder(resp.Body).Decode(&gR)
	if err != nil {
		return result, fmt.Errorf("can not parse json response: %w", err)
	}
	if len(gR.Value) == 0 {
		return result, semerr.NotFoundf("parent group for %q not found", groupInfo.Name)
	}
	return c.internalGroupToGroupInfo(gR.Value[0]), nil
}
