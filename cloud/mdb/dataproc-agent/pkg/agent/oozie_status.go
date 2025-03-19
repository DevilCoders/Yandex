package agent

import (
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type oozieAdminStatus struct {
	SystemModeRaw string `json:"systemMode"`
}

func parseOozieResponse(body []byte) (models.OozieInfo, error) {
	var err error
	var systemMode oozieAdminStatus
	if err = json.Unmarshal(body, &systemMode); err != nil {
		return models.OozieInfo{}, xerrors.Errorf("parse error %q: %w", body, err)
	}
	if systemMode.SystemModeRaw != "NORMAL" {
		return models.OozieInfo{}, xerrors.Errorf("Oozie has incorrect mode %s", systemMode.SystemModeRaw)
	}
	return models.OozieInfo{Available: true}, nil
}

// FetchOozieInfo fetches and parses info from server
func FetchOozieInfo(ctx context.Context, url string) (models.OozieInfo, error) {
	client := &http.Client{}
	url = url + "/oozie/v1/admin/status"
	request, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return models.OozieInfo{}, xerrors.Errorf("failed to build request: %w", err)
	}
	request = request.WithContext(ctx)

	resp, err := client.Do(request)
	if err != nil {
		return models.OozieInfo{}, xerrors.Errorf("failed to get status from server %q: %w", url, err)
	}
	defer func() { _ = resp.Body.Close() }()
	if resp.StatusCode != http.StatusOK {
		return models.OozieInfo{}, xerrors.Errorf("server responded with an error: %s", resp.Status)
	}
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return models.OozieInfo{}, xerrors.Errorf("failed to read response body: %w", err)
	}

	return parseOozieResponse(body)
}
