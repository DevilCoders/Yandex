package agent

import (
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"
	"strings"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func parseHbaseResponse(body []byte) (models.HbaseInfo, error) {
	var info models.HbaseInfo
	if err := json.Unmarshal(body, &info); err != nil {
		return models.HbaseInfo{}, xerrors.Errorf("parse error %q: %w", body, err)
	}
	info.Available = true

	for i, node := range info.LiveNodes {
		if idx := strings.Index(node.Name, ":"); idx > -1 {
			node.Name = node.Name[0:idx]
			info.LiveNodes[i] = node
		}
	}

	for i, node := range info.DeadNodes {
		if idx := strings.Index(node.Name, ":"); idx > -1 {
			node.Name = node.Name[0:idx]
			info.DeadNodes[i] = node
		}
	}

	return info, nil
}

// FetchHbaseInfo fetches and parses info from rest api
func FetchHbaseInfo(ctx context.Context, url string) (models.HbaseInfo, error) {
	client := &http.Client{}
	url = url + "/status/cluster"
	request, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return models.HbaseInfo{}, xerrors.Errorf("failed to build request: %w", err)
	}
	request.Header.Set("Accept", "application/json")
	request = request.WithContext(ctx)

	resp, err := client.Do(request)
	if err != nil {
		return models.HbaseInfo{}, xerrors.Errorf("failed to get status from server %q: %w", url, err)
	}
	defer func() { _ = resp.Body.Close() }()
	if resp.StatusCode != http.StatusOK {
		return models.HbaseInfo{}, xerrors.Errorf("server responded with an error: %s", resp.Status)
	}
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return models.HbaseInfo{}, xerrors.Errorf("failed to read response body: %w", err)
	}

	return parseHbaseResponse(body)
}
