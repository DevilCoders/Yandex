package sdk

import (
	"context"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
)

////////////////////////////////////////////////////////////////////////////////

const (
	SolomonURL = "https://solomon.yandex-team.ru/api/v2/projects"
)

type AuthType string

const (
	OAuth AuthType = "OAuth"
	IAM   AuthType = "IAM"
)

////////////////////////////////////////////////////////////////////////////////

type SolomonClientIface interface {
	ListAlerts(
		ctx context.Context,
	) ([]AlertID, error)

	GetAlert(
		ctx context.Context,
		alertID AlertID,
	) (Alert, error)

	AddAlert(
		ctx context.Context,
		alert Alert,
	) (Alert, error)

	UpdateAlert(
		ctx context.Context,
		alert Alert,
	) (Alert, error)

	DeleteAlert(
		ctx context.Context,
		alertID AlertID,
	) error

	ListChannels(
		ctx context.Context,
	) ([]ChannelID, error)

	GetChannel(
		ctx context.Context,
		channelID ChannelID,
	) (Channel, error)

	AddChannel(
		ctx context.Context,
		channel Channel,
	) (Channel, error)

	UpdateChannel(
		ctx context.Context,
		channel Channel,
	) (Channel, error)

	DeleteChannel(
		ctx context.Context,
		channelID ChannelID,
	) error

	ListClusters(
		ctx context.Context,
	) ([]ClusterID, error)

	GetCluster(
		ctx context.Context,
		clusterID ClusterID,
	) (Cluster, error)

	AddCluster(
		ctx context.Context,
		cluster Cluster,
	) (Cluster, error)

	UpdateCluster(
		ctx context.Context,
		cluster Cluster,
	) (Cluster, error)

	DeleteCluster(
		ctx context.Context,
		clusterID ClusterID,
	) error

	ListServices(
		ctx context.Context,
	) ([]ServiceID, error)

	GetService(
		ctx context.Context,
		serviceID ServiceID,
	) (Service, error)

	AddService(
		ctx context.Context,
		service Service,
	) (Service, error)

	UpdateService(
		ctx context.Context,
		service Service,
	) (Service, error)

	DeleteService(
		ctx context.Context,
		serviceID ServiceID,
	) error

	ListShards(
		ctx context.Context,
	) ([]ShardID, error)

	GetShard(
		ctx context.Context,
		shardID ShardID,
	) (Shard, error)

	AddShard(
		ctx context.Context,
		shard Shard,
	) (Shard, error)

	UpdateShard(
		ctx context.Context,
		shard Shard,
	) (Shard, error)

	DeleteShard(
		ctx context.Context,
		shardID ShardID,
	) error

	ListDashboards(
		ctx context.Context,
	) ([]DashboardID, error)

	GetDashboard(
		ctx context.Context,
		dashboardID DashboardID,
	) (Dashboard, error)

	AddDashboard(
		ctx context.Context,
		dashboard Dashboard,
	) (Dashboard, error)

	UpdateDashboard(
		ctx context.Context,
		dashboard Dashboard,
	) (Dashboard, error)

	DeleteDashboard(
		ctx context.Context,
		dashboardID DashboardID,
	) error

	ListGraphs(
		ctx context.Context,
	) ([]GraphID, error)

	GetGraph(
		ctx context.Context,
		GraphID GraphID,
	) (Graph, error)

	AddGraph(
		ctx context.Context,
		Graph Graph,
	) (Graph, error)

	UpdateGraph(
		ctx context.Context,
		Graph Graph,
	) (Graph, error)

	DeleteGraph(
		ctx context.Context,
		GraphID GraphID,
	) error
}

////////////////////////////////////////////////////////////////////////////////

type solomonClient struct {
	http *http.Client

	projectID string
	url       string
	token     string

	authType AuthType
}

////////////////////////////////////////////////////////////////////////////////

func (s *solomonClient) executeRequest(
	ctx context.Context,
	tag string,
	method string,
	url string,
	requestBody io.Reader,
) ([]byte, error) {

	req, err := http.NewRequest(method, url, requestBody)
	if err != nil {
		return nil, fmt.Errorf(tag+". Can't create request: %w", err)
	}

	if s.authType == "IAM" {
		req.Header.Add("Authorization", "Bearer "+s.token)
	} else {
		req.Header.Add("Authorization", "OAuth "+s.token)
	}
	req.Header.Add("Content-Type", "application/json;charset=UTF-8")
	req.Header.Add("Accept", "application/json")

	resp, err := s.http.Do(req)
	if err != nil {
		return nil, fmt.Errorf(tag+". Request error: %w", err)
	}

	defer resp.Body.Close()
	responseBody, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, fmt.Errorf(tag+". Read body error: %w", err)
	}

	if resp.StatusCode == 404 {
		return []byte("{}"), nil
	}

	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		return nil, fmt.Errorf(tag+". Bad response: %v", resp)
	}

	return responseBody, nil
}

////////////////////////////////////////////////////////////////////////////////

func NewClient(
	projectID string,
	url string,
	token string,
	authType AuthType,
) SolomonClientIface {

	return &solomonClient{
		http:      &http.Client{},
		projectID: projectID,
		url:       url,
		token:     token,
		authType:  authType,
	}
}
