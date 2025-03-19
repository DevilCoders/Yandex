package client

import (
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"
	urlpkg "net/url"

	internal "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/internal-api"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/role"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/service"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Config represents client configuration
type Config struct {
	URL          string                   `json:"url" yaml:"url"`
	AccessID     secret.String            `json:"access_id" yaml:"access_id"`
	AccessSecret secret.String            `json:"access_secret" yaml:"access_secret"`
	CAPath       string                   `json:"capath" yaml:"capath"`
	Transport    httputil.TransportConfig `json:"transport" yaml:"transport"`
}

// DefaultConfig return default config for internal api http client
func DefaultConfig() Config {
	return Config{
		Transport: httputil.TransportConfig{
			Retry: retry.Config{
				MaxRetries: 5,
			},
		},
	}
}

type client struct {
	cfg        Config
	httpClient *http.Client
}

// New constructor for internal api http client
func New(cfg Config, logger log.Logger) (internal.InternalAPI, error) {
	if cfg.CAPath != "" {
		cfg.Transport.TLS.CAFile = cfg.CAPath
	}
	rt, err := httputil.NewTransport(cfg.Transport, logger)
	if err != nil {
		return nil, xerrors.Errorf("failed to create http transport: %w", err)
	}

	return &client{
		cfg:        cfg,
		httpClient: &http.Client{Transport: rt},
	}, nil
}

type subclusterInfo struct {
	Subcid              string   `json:"subcid"`
	Role                string   `json:"role"`
	Services            []string `json:"services"`
	Hosts               []string `json:"hosts"`
	HostsCount          int64    `json:"hosts_count"`
	InstanceGroupID     string   `json:"instance_group_id"`
	DecommissionTimeout int64    `json:"decommission_timeout"`
}

type topologyInfo struct {
	Revision         int64            `json:"revision"`
	FolderID         string           `json:"folder_id"`
	Services         []string         `json:"services"`
	MainSubcluster   subclusterInfo   `json:"subcluster_main"`
	Subclusters      []subclusterInfo `json:"subclusters"`
	UIProxy          bool             `json:"ui_proxy"`
	ServiceAccountID string           `json:"service_account_id"`
}

var (
	serviceMap = map[string]service.Service{
		"hdfs":      service.Hdfs,
		"yarn":      service.Yarn,
		"mapreduce": service.Mapreduce,
		"hive":      service.Hive,
		"tez":       service.Tez,
		"zookeeper": service.Zookeeper,
		"hbase":     service.Hbase,
		"sqoop":     service.Sqoop,
		"flume":     service.Flume,
		"spark":     service.Spark,
		"zeppelin":  service.Zeppelin,
		"oozie":     service.Oozie,
		"livy":      service.Livy,
	}

	serviceIgnore = map[service.Service]struct{}{
		service.Tez:       {},
		service.Flume:     {},
		service.Sqoop:     {},
		service.Spark:     {},
		service.Zeppelin:  {},
		service.Mapreduce: {},
		service.Livy:      {},
	}

	rolesMap = map[string]role.Role{
		"hadoop_cluster.masternode":  role.Main,
		"hadoop_cluster.datanode":    role.Data,
		"hadoop_cluster.computenode": role.Compute,
	}
)

func parseServices(services []string) ([]service.Service, error) {
	result := make([]service.Service, 0, len(services))
	for _, serviceInput := range services {
		srvc, ok := serviceMap[serviceInput]
		if !ok {
			return result, xerrors.Errorf("wrong service name: %s", serviceInput)
		}
		if _, ignore := serviceIgnore[srvc]; ignore {
			continue
		}
		result = append(result, srvc)
	}
	return result, nil
}

func parseSubcluster(subcluster subclusterInfo) (models.SubclusterTopology, error) {
	servicesParsed, err := parseServices(subcluster.Services)
	if err != nil {
		return models.SubclusterTopology{}, xerrors.Errorf("failed to parse services: %w", err)
	}

	roleParsed, ok := rolesMap[subcluster.Role]
	if !ok {
		return models.SubclusterTopology{}, xerrors.Errorf("wrong role name: %s", subcluster.Role)
	}

	return models.SubclusterTopology{
		Subcid:              subcluster.Subcid,
		Role:                roleParsed,
		Services:            servicesParsed,
		Hosts:               subcluster.Hosts,
		MinHostsCount:       subcluster.HostsCount,
		InstanceGroupID:     subcluster.InstanceGroupID,
		DecommissionTimeout: subcluster.DecommissionTimeout,
	}, nil
}

func parseClusterTopology(cid string, body []byte) (models.ClusterTopology, error) {
	var err error
	var apiResp topologyInfo
	if err = json.Unmarshal(body, &apiResp); err != nil {
		return models.ClusterTopology{}, xerrors.Errorf("parse error %q: %w", body, err)
	}

	servicesParsed, err := parseServices(apiResp.Services)
	if err != nil {
		return models.ClusterTopology{}, xerrors.Errorf("failed to parse services: %w", err)
	}

	mainSubcluster, err := parseSubcluster(apiResp.MainSubcluster)
	if err != nil {
		return models.ClusterTopology{}, xerrors.Errorf("failed to parse main subcluster: %w", err)
	}
	subclustersParsed := make([]models.SubclusterTopology, 0, len(apiResp.Subclusters)+1)
	subclustersParsed = append(subclustersParsed, mainSubcluster)
	for _, subcluster := range apiResp.Subclusters {
		subclusterParsed, err := parseSubcluster(subcluster)
		if err != nil {
			return models.ClusterTopology{}, xerrors.Errorf("failed to parse subcluster %q: %w", subcluster.Subcid, err)
		}
		subclustersParsed = append(subclustersParsed, subclusterParsed)
	}

	return models.ClusterTopology{
		Cid:              cid,
		Revision:         apiResp.Revision,
		FolderID:         apiResp.FolderID,
		Services:         servicesParsed,
		Subclusters:      subclustersParsed,
		UIProxy:          apiResp.UIProxy,
		ServiceAccountID: apiResp.ServiceAccountID,
	}, nil
}

// GetClusterTopology fetches cluster topology from internal api
func (c *client) GetClusterTopology(ctx context.Context, cid string) (models.ClusterTopology, error) {
	url := c.cfg.URL + "/mdb/hadoop/1.0/clusters/" + urlpkg.PathEscape(cid) + "/topology"
	request, err := http.NewRequest(http.MethodGet, url, nil)
	if err != nil {
		return models.ClusterTopology{}, xerrors.Errorf("failed to build request: %w", err)
	}

	body, err := c.sendRequest(ctx, request)
	if err != nil {
		return models.ClusterTopology{}, xerrors.Errorf("failed to get topology for cid '%s': %w", cid, err)
	}

	return parseClusterTopology(cid, body)
}

func (c *client) sendRequest(ctx context.Context, request *http.Request) ([]byte, error) {
	request.Header.Set("Access-Id", c.cfg.AccessID.Unmask())
	request.Header.Set("Access-Secret", c.cfg.AccessSecret.Unmask())
	request = request.WithContext(ctx)

	resp, err := c.httpClient.Do(request)
	if err != nil {
		return nil, err
	}
	defer func() { _ = resp.Body.Close() }()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, xerrors.Errorf("failed to read response body: %w", err)
	}
	if resp.StatusCode == http.StatusNotFound {
		intapiError := struct {
			Message string
		}{}
		err = json.Unmarshal(body, &intapiError)
		if err != nil {
			return nil, xerrors.Errorf("failed to parse json response of intapi: %w", err)
		}
		return nil, semerr.NotFoundf(intapiError.Message)
	}
	if resp.StatusCode != http.StatusOK {
		return nil, xerrors.Errorf("server responded with an error: %s", resp.Status)
	}

	return body, nil
}
