package agent

import (
	"context"
	"encoding/json"
	"math"
	"net/http"
	"time"

	"a.yandex-team.ru/library/go/core/log"
)

type MetricsServerConfig struct {
	Timeout                      time.Duration `json:"timeout" yaml:"timeout"`
	ContainersQueueSizeThreshold int64         `json:"containers_queue_size_threshold" yaml:"containers_queue_size_threshold"`
	YARNMetricsLocation          string        `json:"yarn_metrics_location" yaml:"yarn_metrics_location"`
	ServerLocation               string        `json:"server_location" yaml:"server_location"`
	Disabled                     bool          `json:"disabled" yaml:"disabled"`
}

func MetricsServerDefaultConfig() MetricsServerConfig {
	return MetricsServerConfig{
		Timeout:                      5 * time.Second,
		ContainersQueueSizeThreshold: 2,
		YARNMetricsLocation:          "",
		ServerLocation:               "localhost:8090",
	}
}

type MetricsServer struct {
	cfg    MetricsServerConfig
	app    App
	meta   MetaInfo
	logger log.Logger
}

func NewMetricsServer(cfg MetricsServerConfig, app App, meta MetaInfo, logger log.Logger) *MetricsServer {
	if cfg.YARNMetricsLocation == "" {
		cfg.YARNMetricsLocation = app.cfg.AgentCfg.Health.YARNAPIURL
	}
	s := &MetricsServer{
		cfg:    cfg,
		app:    app,
		meta:   meta,
		logger: logger,
	}
	return s
}

type ClusterMetricsResponse struct {
	NeededAutoscalingSubclusterNodeNumber int64 `json:"dataproc.cluster.neededAutoscalingNodesNumber"`
}

func getNumberOfNeededAutoscalingNodes(
	activeNodes int64,
	staticNodes int64,
	containersRunning int64,
	containersPending int64,
	containersQueueSizeThreshold int64,
) int64 {
	if containersPending == 0 {
		return 0
	}
	autoscalingNodesNumber := activeNodes - staticNodes
	if containersPending < containersQueueSizeThreshold {
		return autoscalingNodesNumber
	}

	runningContainersPerNode := float64(containersRunning)
	if autoscalingNodesNumber > 0 {
		runningContainersPerNode = runningContainersPerNode / float64(autoscalingNodesNumber)
	}

	neededAutoscalingSubclusterNodeNumber := autoscalingNodesNumber
	if runningContainersPerNode > 0 {
		neededAutoscalingSubclusterNodeNumber += int64(math.Ceil(
			float64(containersPending) / runningContainersPerNode,
		))
	}

	return neededAutoscalingSubclusterNodeNumber
}

func (server *MetricsServer) DataprocMetrics(w http.ResponseWriter, _ *http.Request) {
	ctx, cancel := context.WithTimeout(context.Background(), server.cfg.Timeout)
	defer cancel()
	clusterMetrics, err := FetchYarnClusterMetrics(
		ctx,
		server.cfg.YARNMetricsLocation,
	)
	if err != nil {
		server.logger.Errorf("error while fetching yarn cluster metrics %v", err)
	}
	neededAutoscalingSubclusterNodeNumber := getNumberOfNeededAutoscalingNodes(
		clusterMetrics.ClusterMetrics.ActiveNodes,
		server.meta.StaticNodesNumber,
		clusterMetrics.ClusterMetrics.ContainersAllocated,
		clusterMetrics.ClusterMetrics.ContainersPending,
		server.cfg.ContainersQueueSizeThreshold,
	)
	response, err := json.Marshal(ClusterMetricsResponse{
		NeededAutoscalingSubclusterNodeNumber: neededAutoscalingSubclusterNodeNumber,
	})
	if err != nil {
		server.logger.Errorf("error while serializing json %v", err)
	}
	server.logger.Infof(
		"ActiveNodes=%d StaticNodes=%d ContainersAllocated=%d ContainersPending=%d ContainersQueueSizeThreshold=%d",
		clusterMetrics.ClusterMetrics.ActiveNodes,
		server.meta.StaticNodesNumber,
		clusterMetrics.ClusterMetrics.ContainersAllocated,
		clusterMetrics.ClusterMetrics.ContainersPending,
		server.cfg.ContainersQueueSizeThreshold,
	)
	server.logger.Infof("DataprocMetrics=%s", response)
	_, err = w.Write(response)
	if err != nil {
		server.logger.Errorf("error while writing metrics response %v", err)
	}
}

func (server *MetricsServer) Listen() {
	http.HandleFunc("/metrics", server.DataprocMetrics)
	err := http.ListenAndServe(server.cfg.ServerLocation, nil)
	if err != nil {
		server.logger.Errorf("error while binding port %s : %v", server.cfg.ServerLocation, err)
	}
}
