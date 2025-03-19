package agent

import (
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type YarnClusterMetricsResponse struct {
	ClusterMetrics ClusterMetrics `json:"clusterMetrics"`
}

type ClusterMetrics struct {
	ReservedMB            int64 `json:"reservedMB"`
	AvailableMB           int64 `json:"availableMB"`
	AllocatedMB           int64 `json:"allocatedMB"`
	ReservedVirtualCores  int64 `json:"reservedVirtualCores"`
	AllocatedVirtualCores int64 `json:"allocatedVirtualCores"`
	AvailableVirtualCores int64 `json:"availableVirtualCores"`
	ContainersAllocated   int64 `json:"containersAllocated"`
	ContainersReserved    int64 `json:"containersReserved"`
	ContainersPending     int64 `json:"containersPending"`
	ActiveNodes           int64 `json:"activeNodes"`
}

func parseYarnClusterMetricsResponse(body []byte) (YarnClusterMetricsResponse, error) {
	var err error
	var response YarnClusterMetricsResponse
	if err = json.Unmarshal(body, &response); err != nil {
		return YarnClusterMetricsResponse{}, xerrors.Errorf("parse error %q: %w", body, err)
	}
	return response, nil
}

func FetchYarnClusterMetrics(ctx context.Context, url string) (YarnClusterMetricsResponse, error) {
	client := &http.Client{}
	url = url + "/ws/v1/cluster/metrics"

	request, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return YarnClusterMetricsResponse{}, xerrors.Errorf("failed to build request: %w", err)
	}
	request = request.WithContext(ctx)

	resp, err := client.Do(request)
	if err != nil {
		return YarnClusterMetricsResponse{}, xerrors.Errorf("failed to get status from server %q: %w", url, err)
	}
	defer func() { _ = resp.Body.Close() }()
	if resp.StatusCode != http.StatusOK {
		return YarnClusterMetricsResponse{}, xerrors.Errorf("server responded with an error: %s", resp.Status)
	}
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return YarnClusterMetricsResponse{}, xerrors.Errorf("failed to read response body: %w", err)
	}

	return parseYarnClusterMetricsResponse(body)
}
