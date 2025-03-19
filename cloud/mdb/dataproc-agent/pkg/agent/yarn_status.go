package agent

import (
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type YarnAPIResponse struct {
	Nodes YarnAPIResponseNodes `json:"nodes"`
}

type YarnAPIResponseNodes struct {
	LiveNodes []models.YARNNodeInfo `json:"node"`
}

func parseYarnResponse(body []byte) (models.YARNInfo, error) {
	var err error
	var response YarnAPIResponse
	if err = json.Unmarshal(body, &response); err != nil {
		return models.YARNInfo{}, xerrors.Errorf("parse error %q: %w", body, err)
	}
	yarnNodes := make(map[string]models.YARNNodeInfo)
	// remove duplicate host entries
	for _, node := range response.Nodes.LiveNodes {
		if sameNode, exists := yarnNodes[node.Name]; exists {
			if node.UpdateTime > sameNode.UpdateTime {
				yarnNodes[node.Name] = node
			}
		} else {
			yarnNodes[node.Name] = node
		}
	}
	liveNodes := []models.YARNNodeInfo{}
	for _, node := range yarnNodes {
		liveNodes = append(liveNodes, node)
	}
	return models.YARNInfo{Available: true, LiveNodes: liveNodes}, nil
}

// FetchYARNInfo fetches and parses info from jmx server
func FetchYARNInfo(ctx context.Context, url string) (models.YARNInfo, error) {
	client := &http.Client{}
	url = url + "/ws/v1/cluster/nodes"
	request, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return models.YARNInfo{}, xerrors.Errorf("failed to build request: %w", err)
	}
	request = request.WithContext(ctx)

	resp, err := client.Do(request)
	if err != nil {
		return models.YARNInfo{}, xerrors.Errorf("failed to get status from server %q: %w", url, err)
	}
	defer func() { _ = resp.Body.Close() }()
	if resp.StatusCode != http.StatusOK {
		return models.YARNInfo{}, xerrors.Errorf("server responded with an error: %s", resp.Status)
	}
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return models.YARNInfo{}, xerrors.Errorf("failed to read response body: %w", err)
	}

	return parseYarnResponse(body)
}
