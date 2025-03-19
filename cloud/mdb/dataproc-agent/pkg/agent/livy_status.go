package agent

import (
	"context"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// FetchLivyInfo fetches and parses info from server
func FetchLivyInfo(ctx context.Context, url string) (models.LivyInfo, error) {
	client := &http.Client{}
	url = url + "/sessions"
	request, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return models.LivyInfo{}, xerrors.Errorf("failed to build request: %w", err)
	}
	request = request.WithContext(ctx)

	resp, err := client.Do(request)
	if err != nil {
		return models.LivyInfo{}, xerrors.Errorf("failed to get status from server %q: %w", url, err)
	}
	defer func() { _ = resp.Body.Close() }()
	if resp.StatusCode != http.StatusOK {
		return models.LivyInfo{}, xerrors.Errorf("server responded with an error: %s", resp.Status)
	}
	_, err = ioutil.ReadAll(resp.Body)
	if err != nil {
		return models.LivyInfo{}, xerrors.Errorf("failed to read response body: %w", err)
	}

	return models.LivyInfo{Available: true}, nil
}
