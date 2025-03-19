package sdk

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
)

type ClusterID = string

type ClusterConductorGroup struct {
	Group  string   `json:"group"`
	Labels []string `json:"labels"`
}

type ClustersCloudDNS struct {
	Env    string   `json:"env"`
	Name   string   `json:"name"`
	Labels []string `json:"labels"`
}

type Host struct {
	URLPattern string   `json:"urlPattern"`
	Ranges     string   `json:"ranges"`
	DC         string   `json:"dc"`
	Labels     []string `json:"labels"`
}

type Cluster struct {
	ID        string `json:"id"`
	ProjectID string `json:"projectId"`
	Name      string `json:"name"`
	Version   uint   `json:"version"`

	Hosts           []Host                  `json:"hosts"`
	ConductorGroups []ClusterConductorGroup `json:"conductorGroups"`
	CloudDNS        []ClustersCloudDNS      `json:"cloudDns"`
	SensorsTTLDays  uint                    `json:"sensorsTtlDays"`
	TvmDestID       string                  `json:"tvmDestId"`
	UseFqdn         bool                    `json:"useFqdn"`
}

type listClustersItem struct {
	ID string `json:"id"`
}

type listPage struct {
	PagesCount uint `json:"pagesCount"`
	TotalCount uint `json:"totalCount"`
	PageSize   uint `json:"pageSize"`
	Current    uint `json:"current"`
}

type listClustersResponse struct {
	Items []listClustersItem `json:"result"`
	Page  listPage           `json:"page"`
}

func (s *solomonClient) listClusters(
	ctx context.Context,
	pageNum uint,
) ([]ClusterID, listPage, error) {

	url := s.url + "/" + s.projectID + "/clusters?pageSize=1000"
	url += "&page=" + fmt.Sprint(pageNum)

	body, err := s.executeRequest(ctx, "listClusters", "GET", url, nil)
	if err != nil {
		return nil, listPage{}, err
	}

	r := &listClustersResponse{}
	if err := json.Unmarshal(body, r); err != nil {
		return nil, listPage{}, fmt.Errorf("listClusters. Unmarshal error: %w", err)
	}

	var result []string

	for _, item := range r.Items {
		result = append(result, item.ID)
	}

	return result, r.Page, nil
}

func (s *solomonClient) ListClusters(
	ctx context.Context,
) ([]ClusterID, error) {

	var result []string

	pageNum := uint(0)
	for {
		clusters, page, err := s.listClusters(ctx, pageNum)
		if err != nil {
			return result, err
		}

		result = append(result, clusters...)
		pageNum = page.Current + 1

		if pageNum >= page.PagesCount {
			break
		}
	}

	return result, nil
}

func (s *solomonClient) GetCluster(
	ctx context.Context,
	clusterID ClusterID,
) (Cluster, error) {

	url := s.url + "/" + s.projectID + "/clusters/" + clusterID

	body, err := s.executeRequest(ctx, "GetClusters", "GET", url, nil)
	if err != nil {
		return Cluster{}, err
	}

	r := &Cluster{}
	if err := json.Unmarshal(body, r); err != nil {
		return Cluster{}, fmt.Errorf("GetClusters. Unmarshal error: %w", err)
	}

	return *r, nil
}

func (s *solomonClient) AddCluster(
	ctx context.Context,
	cluster Cluster,
) (Cluster, error) {

	url := s.url + "/" + s.projectID + "/clusters"

	payload, err := json.Marshal(cluster)
	if err != nil {
		return Cluster{}, err
	}

	body, err := s.executeRequest(
		ctx,
		"AddCluster",
		"POST",
		url,
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return Cluster{}, err
	}

	r := &Cluster{}
	if err := json.Unmarshal(body, r); err != nil {
		return Cluster{}, err
	}

	return *r, nil
}

func (s *solomonClient) UpdateCluster(
	ctx context.Context,
	cluster Cluster,
) (Cluster, error) {

	url := s.url + "/" + s.projectID + "/clusters/" + cluster.ID

	payload, err := json.Marshal(cluster)
	if err != nil {
		return Cluster{}, err
	}

	body, err := s.executeRequest(
		ctx,
		"UpdateCluster",
		"PUT",
		url,
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return Cluster{}, err
	}

	r := &Cluster{}
	if err := json.Unmarshal(body, r); err != nil {
		return Cluster{}, err
	}

	return *r, nil
}

func (s *solomonClient) DeleteCluster(
	ctx context.Context,
	clusterID ClusterID,
) error {

	url := s.url + "/" + s.projectID + "/clusters/" + clusterID

	_, err := s.executeRequest(ctx, "DeleteCluster", "DELETE", url, nil)
	if err != nil {
		return err
	}

	return nil
}
