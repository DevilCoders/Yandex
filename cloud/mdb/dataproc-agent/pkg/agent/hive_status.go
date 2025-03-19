package agent

import (
	"bytes"
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type hiveInfoBean struct {
	Name  string  `json:"name"`
	Count int64   `json:"Count"`
	Value float64 `json:"Value"`
}

type hiveResp struct {
	Beans []hiveInfoBean `json:"beans"`
}

// getName removes "metrics:name=hs2_" prefix
func getName(bean string) string {
	return bean[17:]
}

func parseHiveResponse(body []byte) (models.HiveInfo, error) {
	// NaN is invalid value for JSON
	body = bytes.Replace(body, []byte(": NaN"), []byte(": null"), -1)

	var jmx hiveResp
	if err := json.Unmarshal(body, &jmx); err != nil {
		return models.HiveInfo{}, xerrors.Errorf("parse error %q: %w", body, err)
	}
	if len(jmx.Beans) == 0 {
		return models.HiveInfo{}, xerrors.New("no bean found, possibly bad request")
	}

	beans := make(map[string]hiveInfoBean)
	for _, bean := range jmx.Beans {
		beans[getName(bean.Name)] = bean
	}

	info := models.HiveInfo{}
	info.Available = true
	info.QueriesSucceeded = beans["succeeded_queries"].Count
	info.QueriesFailed = beans["failed_queries"].Count
	info.QueriesExecuting = beans["executing_queries"].Count
	info.SessionsOpen = int64(beans["open_sessions"].Value)
	info.SessionsActive = int64(beans["active_sessions"].Value)
	return info, nil
}

// FetchHiveInfo fetches and parses info from jmx server
func FetchHiveInfo(ctx context.Context, url string) (models.HiveInfo, error) {
	client := &http.Client{}
	url = url + "/jmx?qry=metrics:name=hs2_*"
	request, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return models.HiveInfo{}, xerrors.Errorf("failed to build request: %w", err)
	}
	request = request.WithContext(ctx)

	resp, err := client.Do(request)
	if err != nil {
		return models.HiveInfo{}, xerrors.Errorf("failed to get status from server %q: %w", url, err)
	}
	defer func() { _ = resp.Body.Close() }()
	if resp.StatusCode != http.StatusOK {
		return models.HiveInfo{}, xerrors.Errorf("server responded with an error: %s", resp.Status)
	}
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return models.HiveInfo{}, xerrors.Errorf("failed to read response body: %w", err)
	}

	return parseHiveResponse(body)
}
