package admin

import (
	"bufio"
	"context"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"path/filepath"
	"strings"
	"testing"

	"a.yandex-team.ru/library/go/test/canon"
	"github.com/stretchr/testify/require"
	"gopkg.in/yaml.v2"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	logutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/log"
	solomon "a.yandex-team.ru/cloud/storage/core/tools/common/go/solomon/sdk"
)

////////////////////////////////////////////////////////////////////////////////

type solomonClientMock struct {
	logutil.WithLog

	alerts     []solomon.Alert
	channels   []solomon.Channel
	services   []solomon.Service
	shards     []solomon.Shard
	clusters   []solomon.Cluster
	dashboards []solomon.Dashboard
	graphs     []solomon.Graph
}

func (s *solomonClientMock) ListAlerts(
	ctx context.Context,
) ([]solomon.AlertID, error) {

	var alertIDs []solomon.AlertID

	for _, alert := range s.alerts {
		alertIDs = append(alertIDs, alert.ID)
	}
	return alertIDs, nil
}

func (s *solomonClientMock) GetAlert(
	ctx context.Context,
	alertID solomon.AlertID,
) (solomon.Alert, error) {

	for _, alert := range s.alerts {
		if alert.ID == alertID {
			return alert, nil
		}
	}

	return solomon.Alert{}, fmt.Errorf("Failed to get alert with ID = %s", alertID)
}

func (s *solomonClientMock) AddAlert(
	ctx context.Context,
	alert solomon.Alert,
) (solomon.Alert, error) {

	s.LogInfo(ctx, "[SOLOMON] ADD alert: %+v", alert)

	return alert, nil
}

func (s *solomonClientMock) UpdateAlert(
	ctx context.Context,
	alert solomon.Alert,
) (solomon.Alert, error) {

	s.LogInfo(ctx, "[SOLOMON] UPDATE alert: %+v", alert)

	return alert, nil
}

func (s *solomonClientMock) DeleteAlert(
	ctx context.Context,
	alertID solomon.AlertID,
) error {

	s.LogInfo(ctx, "[SOLOMON] DELETE alert: %s", alertID)

	return nil
}

func (s *solomonClientMock) ListChannels(
	ctx context.Context,
) ([]solomon.ChannelID, error) {

	var channelIDs []solomon.ChannelID
	return channelIDs, nil
}

func (s *solomonClientMock) GetChannel(
	ctx context.Context,
	channelID solomon.ChannelID,
) (solomon.Channel, error) {

	for _, channel := range s.channels {
		if channel.ID == channelID {
			return channel, nil
		}
	}

	return solomon.Channel{}, fmt.Errorf("failed to get channel with ID = %s", channelID)
}

func (s *solomonClientMock) AddChannel(
	ctx context.Context,
	channel solomon.Channel,
) (solomon.Channel, error) {

	s.LogInfo(ctx, "[SOLOMON] ADD channel: %+v", channel)

	return channel, nil
}

func (s *solomonClientMock) UpdateChannel(
	ctx context.Context,
	channel solomon.Channel,
) (solomon.Channel, error) {

	s.LogInfo(ctx, "[SOLOMON] UPDATE channel: %+v", channel)

	return channel, nil
}

func (s *solomonClientMock) DeleteChannel(
	ctx context.Context,
	channelID solomon.ChannelID,
) error {

	s.LogInfo(ctx, "[SOLOMON] DELETE channel: %s", channelID)

	return nil
}

func (s *solomonClientMock) ListClusters(
	ctx context.Context,
) ([]solomon.ClusterID, error) {
	var clusterIDs []solomon.ClusterID
	return clusterIDs, nil
}

func (s *solomonClientMock) GetCluster(
	ctx context.Context,
	clusterID solomon.ClusterID,
) (solomon.Cluster, error) {

	for _, cluster := range s.clusters {
		if cluster.ID == clusterID {
			return cluster, nil
		}
	}

	return solomon.Cluster{}, fmt.Errorf("failed to get cluster with ID = %s", clusterID)
}

func (s *solomonClientMock) AddCluster(
	ctx context.Context,
	cluster solomon.Cluster,
) (solomon.Cluster, error) {

	s.LogInfo(ctx, "[SOLOMON] ADD cluster: %+v", cluster)

	return cluster, nil
}

func (s *solomonClientMock) UpdateCluster(
	ctx context.Context,
	cluster solomon.Cluster,
) (solomon.Cluster, error) {

	s.LogInfo(ctx, "[SOLOMON] UPDATE cluster: %+v", cluster)

	return cluster, nil
}

func (s *solomonClientMock) DeleteCluster(
	ctx context.Context,
	clusterID solomon.ClusterID,
) error {

	s.LogInfo(ctx, "[SOLOMON] DELETE cluster: %s", clusterID)

	return nil
}

func (s *solomonClientMock) ListServices(
	ctx context.Context,
) ([]solomon.ServiceID, error) {

	var serviceIDs []solomon.ServiceID
	return serviceIDs, nil
}

func (s *solomonClientMock) GetService(
	ctx context.Context,
	serviceID solomon.ServiceID,
) (solomon.Service, error) {

	for _, service := range s.services {
		if service.ID == serviceID {
			return service, nil
		}
	}

	return solomon.Service{}, fmt.Errorf("failed to get service with ID = %s", serviceID)
}

func (s *solomonClientMock) AddService(
	ctx context.Context,
	service solomon.Service,
) (solomon.Service, error) {

	s.LogInfo(ctx, "[SOLOMON] ADD service: %+v", service)

	return service, nil
}

func (s *solomonClientMock) UpdateService(
	ctx context.Context,
	service solomon.Service,
) (solomon.Service, error) {

	s.LogInfo(ctx, "[SOLOMON] UPDATE service: %+v", service)

	return service, nil
}

func (s *solomonClientMock) DeleteService(
	ctx context.Context,
	serviceID solomon.ServiceID,
) error {

	s.LogInfo(ctx, "[SOLOMON] DELETE service: %s", serviceID)

	return nil
}

func (s *solomonClientMock) ListShards(
	ctx context.Context,
) ([]solomon.ShardID, error) {

	var shardIDs []solomon.ShardID
	return shardIDs, nil
}

func (s *solomonClientMock) GetShard(
	ctx context.Context,
	shardID solomon.ShardID,
) (solomon.Shard, error) {

	for _, shard := range s.shards {
		if shard.ID == shardID {
			return shard, nil
		}
	}

	return solomon.Shard{}, fmt.Errorf("failed to get shard with ID = %s", shardID)
}

func (s *solomonClientMock) AddShard(
	ctx context.Context,
	shard solomon.Shard,
) (solomon.Shard, error) {

	s.LogInfo(ctx, "[SOLOMON] ADD shard: %+v", shard)

	return shard, nil
}

func (s *solomonClientMock) UpdateShard(
	ctx context.Context,
	shard solomon.Shard,
) (solomon.Shard, error) {

	s.LogInfo(ctx, "[SOLOMON] UPDATE shard: %+v", shard)

	return shard, nil
}

func (s *solomonClientMock) DeleteShard(
	ctx context.Context,
	shardID solomon.ShardID,
) error {

	s.LogInfo(ctx, "[SOLOMON] DELETE shard: %s", shardID)

	return nil
}

func (s *solomonClientMock) ListDashboards(
	ctx context.Context,
) ([]solomon.DashboardID, error) {

	var dashboardIDs []solomon.DashboardID
	return dashboardIDs, nil
}

func (s *solomonClientMock) GetDashboard(
	ctx context.Context,
	dashboardID solomon.DashboardID,
) (solomon.Dashboard, error) {

	for _, dashboard := range s.dashboards {
		if dashboard.ID == dashboardID {
			return dashboard, nil
		}
	}

	return solomon.Dashboard{}, fmt.Errorf("failed to get dashboard with ID = %s", dashboardID)
}

func (s *solomonClientMock) AddDashboard(
	ctx context.Context,
	dashboard solomon.Dashboard,
) (solomon.Dashboard, error) {

	s.LogInfo(ctx, "[SOLOMON] ADD dashboard: %+v", dashboard)

	return dashboard, nil
}

func (s *solomonClientMock) UpdateDashboard(
	ctx context.Context,
	dashboard solomon.Dashboard,
) (solomon.Dashboard, error) {

	s.LogInfo(ctx, "[SOLOMON] UPDATE dashboard: %+v", dashboard)

	return dashboard, nil
}

func (s *solomonClientMock) DeleteDashboard(
	ctx context.Context,
	dashboardID solomon.DashboardID,
) error {

	s.LogInfo(ctx, "[SOLOMON] DELETE dashboard: %s", dashboardID)

	return nil
}

func (s *solomonClientMock) ListGraphs(
	ctx context.Context,
) ([]solomon.GraphID, error) {

	var graphIDs []solomon.GraphID
	return graphIDs, nil
}

func (s *solomonClientMock) GetGraph(
	ctx context.Context,
	graphID solomon.GraphID,
) (solomon.Graph, error) {

	for _, graph := range s.graphs {
		if graph.ID == graphID {
			return graph, nil
		}
	}

	return solomon.Graph{}, fmt.Errorf("failed to get Graph with ID = %s", graphID)
}

func (s *solomonClientMock) AddGraph(
	ctx context.Context,
	graph solomon.Graph,
) (solomon.Graph, error) {

	s.LogInfo(ctx, "[SOLOMON] ADD Graph: %+v", graph)

	return graph, nil
}

func (s *solomonClientMock) UpdateGraph(
	ctx context.Context,
	graph solomon.Graph,
) (solomon.Graph, error) {

	s.LogInfo(ctx, "[SOLOMON] UPDATE Graph: %+v", graph)

	return graph, nil
}

func (s *solomonClientMock) DeleteGraph(
	ctx context.Context,
	graphID solomon.GraphID,
) error {

	s.LogInfo(ctx, "[SOLOMON] DELETE Graph: %s", graphID)

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func TestSetAlert(t *testing.T) {
	opts := &Options{
		ShowDiff:     false,
		Remove:       true,
		ApplyChanges: true,
	}

	config := &ServiceConfig{
		SolomonProjectID: "nbs",
		Default: Defaults{
			AlertState:      "MUTED",
			AlertWindowSecs: 180,
			AlertDelaySecs:  120,
		},
		Clusters: []ClusterConfig{
			ClusterConfig{
				ID:             "preprod_vla",
				Name:           "[PRE-PROD][VLA]",
				SolomonCluster: "yandexcloud_preprod",
				Cluster:        "preprod",
				Zone:           "vla",
				Hosts:          "vla04-*",
			},
			ClusterConfig{
				ID:             "preprod_sas",
				Name:           "[PRE-PROD][SAS]",
				SolomonCluster: "yandexcloud_preprod",
				Cluster:        "preprod",
				Zone:           "sas",
				Hosts:          "sas09-*",
			},
			ClusterConfig{
				ID:             "preprod_myt",
				Name:           "[PRE-PROD][MYT]",
				SolomonCluster: "yandexcloud_preprod",
				Cluster:        "preprod",
				Zone:           "myt",
				Hosts:          "myt*",
			},
		},
	}

	alertForSkipping := solomon.Alert{
		ID:            "preprod_vla_nbs_test_alert",
		ProjectID:     "nbs",
		Name:          "[PRE-PROD][VLA] NBS Test Alert",
		Description:   "test alert description",
		State:         "MUTED",
		Version:       1,
		GroupByLabels: []string{"shardId", "hosts"},
		Channels: []solomon.AlertChannel{
			solomon.AlertChannel{
				ID: "telegram",
			},
			solomon.AlertChannel{
				ID: "juggler",
				Config: solomon.AlertChannelConfig{
					NotifyAboutStatuses: []string{"OK", "ALARM", "WARN"},
					RepeatDelaySecs:     100,
				},
			},
		},
		Type: solomon.AlertType{
			Expression: solomon.AlertExpression{
				Program:         "test alert program [yandexcloud_preprod, vla04-*]",
				CheckExpression: "test alert check [yandexcloud_preprod, vla04-*]",
			},
		},
		Annotations: solomon.AlertAnnotations{
			Host:               "solomon-alert-cloud_preprod_nbs_vla",
			Tags:               "yc-preprod-nbs-solomon",
			Debug:              "test alert debug",
			Service:            "solomon_alert_nbs_test",
			Content:            "test alert content",
			GraphLink:          "test alert graph link",
			JugglerDescription: "test alert juggler description",
		},
		PeriodMillis: 180000,
		WindowSecs:   180,
		DelaySeconds: 120,
		DelaySecs:    120,
	}

	alertForUpdating := alertForSkipping
	alertForUpdating.ID = "preprod_sas_nbs_test_alert"

	alertForRemoving := solomon.Alert{
		ID: "preprod_nbs_test_alert",
	}

	alertConfigs := AlertConfigs{
		Alerts: []AlertConfig{
			AlertConfig{
				ID:                  "{{.ID}}_nbs_test_alert",
				Clusters:            []string{"preprod_vla", "preprod_sas", "preprod_myt"},
				Name:                "{{.Name}} NBS Test Alert",
				Description:         "test alert description",
				Debug:               "test alert debug",
				GroupByLabels:       []string{"shardId", "hosts"},
				ResolvedEmptyPolicy: "RESOLVED_EMPTY_OK",
				Channels: []AlertChannel{
					AlertChannel{
						ID: "telegram",
					},
					AlertChannel{
						ID:                  "juggler",
						NotifyAboutStatuses: []string{"OK", "ALARM", "WARN"},
						ReNotification:      100,
					},
				},
				Annotations: &AlertAnnotations{
					Tags:               "yc-{{.Cluster}}-nbs-solomon",
					Host:               "solomon-alert-cloud_{{.Cluster}}_nbs_{{.Zone}}",
					Service:            "solomon_alert_nbs_test",
					Content:            "test alert content",
					GraphLink:          "test alert graph link",
					JugglerDescription: "test alert juggler description",
				},
				Expression: AlertExpression{
					Program: "test alert program [{{.SolomonCluster}}, {{.Hosts}}]",
					Check:   "test alert check [{{.SolomonCluster}}, {{.Hosts}}]",
				},
			},
		},
	}

	data, err := yaml.Marshal(alertConfigs)
	require.NoError(t, err)

	tmpFile, err := ioutil.TempFile("", "alerts")
	require.NoError(t, err)

	file := tmpFile.Name() + ".yaml"
	err = os.Rename(tmpFile.Name(), file)
	require.NoError(t, err)

	err = ioutil.WriteFile(file, data, 0644)
	require.NoError(t, err)

	opts.AlertsPath = tmpFile.Name() + ".yaml"

	tmpDir, err := ioutil.TempDir(os.TempDir(), "canon_")
	require.NoError(t, err)
	defer func() { _ = os.RemoveAll(tmpDir) }()

	canonFileName := func(t *testing.T) string {
		testName := strings.Replace(t.Name(), "/", ".", -1)
		return filepath.Join(tmpDir, testName)
	}

	logFile, err := os.Create(canonFileName(t))
	require.NoError(t, err)
	defer func() { _ = logFile.Close() }()

	logWriter := bufio.NewWriter(logFile)

	logger := nbs.NewLog(
		log.New(
			io.MultiWriter(logWriter, os.Stdout),
			"",
			0,
		),
		nbs.LOG_INFO,
	)

	a := NewAlertManager(
		opts,
		config,
		&solomonClientMock{
			WithLog: logutil.WithLog{Log: logger},
			alerts: []solomon.Alert{
				alertForSkipping,
				alertForUpdating,
				alertForRemoving,
			},
		},
	)

	err = a.Run(context.TODO())
	require.NoError(t, err)

	_ = logWriter.Flush()
	canon.SaveFile(t, canonFileName(t), canon.WithLocal(false))
}
