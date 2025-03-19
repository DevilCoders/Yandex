package agent

import (
	"context"
	"math"
	"sync"
	"sync/atomic"
	"time"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/apiclient"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/decommission"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/library/go/core/log"
)

// HealthConfig describes configuration of health check urls
type HealthConfig struct {
	RequestTimeout time.Duration `json:"request_timeout" yaml:"request_timeout"`
	HDFSAPIURL     string        `json:"hdfs_api_url" yaml:"hdfs_api_url"`
	YARNAPIURL     string        `json:"yarn_api_url" yaml:"yarn_api_url"`
	HIVEAPIURL     string        `json:"hive_api_url" yaml:"hive_api_url"`
	HbaseAPIURL    string        `json:"hbase_api_url" yaml:"hbase_api_url"`
	ZkAPIURL       string        `json:"zookeeper_url" yaml:"zookeeper_url"`
	OozieAPIURL    string        `json:"oozie_url" yaml:"oozie_url"`
	LivyAPIURL     string        `json:"livy_url" yaml:"livy_url"`
}

// Config describes configuration for dataproc-agent
type Config struct {
	Timeout      time.Duration                   `json:"timeout" yaml:"timeout"`
	Health       HealthConfig                    `json:"health" yaml:"health"`
	Cid          string                          `json:"cid" yaml:"cid"`
	Services     []string                        `json:"services" yaml:"services"`
	Decommission decommission.DecommissionConfig `json:"decommission" yaml:"decommission"`
	Disabled     bool                            `json:"disabled" yaml:"disabled"`
	Systemd      bool                            `json:"systemd" yaml:"systemd"`
}

// DefaultConfig returns default config for dataproc-agent
func DefaultConfig() Config {
	return Config{
		Timeout: 5 * time.Second,
		Health: HealthConfig{
			RequestTimeout: 5 * time.Second,
			HDFSAPIURL:     "http://localhost:9870",
			YARNAPIURL:     "http://localhost:8088",
			HIVEAPIURL:     "http://localhost:10002",
			HbaseAPIURL:    "http://localhost:8070",
			ZkAPIURL:       "localhost:2181",
			OozieAPIURL:    "http://localhost:11000",
			LivyAPIURL:     "http://localhost:8998",
		},
		Decommission: decommission.DecommissionConfig{
			YARNExcludeNodesConfigPath: "/etc/hadoop/conf/yarn-nodes.exclude",
			HDFSExcludeNodesConfigPath: "/etc/hadoop/conf/dfs.exclude",
		},
		Systemd: false,
	}
}

// Agent implements dataproc-agent
type Agent struct {
	logger           log.Logger
	apiClient        apiclient.HealthAPI
	cfg              Config
	topologyRevision atomic.Value
	decommissioner   decommission.Decommissioner
	reportCount      int64
}

// NewAgent constructs agent
func NewAgent(cfg Config, apiClient apiclient.HealthAPI, revision int64, logger log.Logger) *Agent {
	a := &Agent{
		cfg:            cfg,
		apiClient:      apiClient,
		logger:         logger,
		decommissioner: decommission.Decommissioner{Logger: logger, Config: cfg.Decommission},
	}
	a.topologyRevision.Store(revision)
	return a
}

func (a *Agent) fetchInfo(ctx context.Context) models.Info {
	info := models.Info{Cid: a.cfg.Cid}
	ctx, cancel := context.WithTimeout(ctx, a.cfg.Health.RequestTimeout)
	defer cancel()

	servicesInfoCh := make(chan interface{}, len(a.cfg.Services))
	var wg sync.WaitGroup

	a.logger.Debugf("AgentCfg.Services: %+v", a.cfg.Services)

	for _, service := range a.cfg.Services {
		switch service {
		case "hdfs":
			wg.Add(1)
			go func() {
				defer wg.Done()
				hdfsInfo, err := FetchHDFSInfo(ctx, a.cfg.Health.HDFSAPIURL)
				if err != nil {
					a.logger.Error(err.Error())
				} else {
					hdfsInfo.RequestedDecommissionHosts = a.decommissioner.HdfsRequestedDecommissionHosts
				}
				a.logger.Debugf("HDFS Info: %+v", hdfsInfo)
				servicesInfoCh <- hdfsInfo
			}()

		case "yarn":
			wg.Add(1)
			go func() {
				defer wg.Done()
				yarnInfo, err := FetchYARNInfo(ctx, a.cfg.Health.YARNAPIURL)
				if err != nil {
					a.logger.Error(err.Error())
				} else {
					yarnInfo.RequestedDecommissionHosts = a.decommissioner.YarnRequestedDecommissionHosts
				}
				a.logger.Debugf("YARN Info: %+v", yarnInfo)
				servicesInfoCh <- yarnInfo
			}()

		case "hive":
			wg.Add(1)
			go func() {
				defer wg.Done()
				hiveInfo, err := FetchHiveInfo(ctx, a.cfg.Health.HIVEAPIURL)
				if err != nil {
					a.logger.Error(err.Error())
				}
				a.logger.Debugf("Hive Info: %+v", hiveInfo)
				servicesInfoCh <- hiveInfo
			}()

		case "hbase":
			wg.Add(1)
			go func() {
				defer wg.Done()
				hbaseInfo, err := FetchHbaseInfo(ctx, a.cfg.Health.HbaseAPIURL)
				if err != nil {
					a.logger.Error(err.Error())
				}
				a.logger.Debugf("Hbase Info: %+v", hbaseInfo)
				servicesInfoCh <- hbaseInfo
			}()

		case "zookeeper":
			wg.Add(1)
			go func() {
				defer wg.Done()
				zkInfo, err := FetchZKInfo(ctx, a.cfg.Health.ZkAPIURL)
				if err != nil {
					a.logger.Error(err.Error())
				}
				a.logger.Debugf("ZK Info: %+v", zkInfo)
				servicesInfoCh <- zkInfo
			}()

		case "oozie":
			wg.Add(1)
			go func() {
				defer wg.Done()
				oozieInfo, err := FetchOozieInfo(ctx, a.cfg.Health.OozieAPIURL)
				if err != nil {
					a.logger.Error(err.Error())
				}
				a.logger.Debugf("Oozie Info: %+v", oozieInfo)
				servicesInfoCh <- oozieInfo
			}()

		case "livy":
			wg.Add(1)
			go func() {
				defer wg.Done()
				livyInfo, err := FetchLivyInfo(ctx, a.cfg.Health.LivyAPIURL)
				if err != nil {
					a.logger.Error(err.Error())
				}
				a.logger.Debugf("Livy Info: %+v", livyInfo)
				servicesInfoCh <- livyInfo
			}()
		}
	}

	wg.Wait()
	close(servicesInfoCh)

	revision, ok := a.topologyRevision.Load().(int64)
	if !ok {
		a.logger.Error("Can't get revision from atomic")
	}

	info.TopologyRevision = revision

	for item := range servicesInfoCh {
		switch v := item.(type) {
		case models.HDFSInfo:
			info.HDFS = v
		case models.YARNInfo:
			info.YARN = v
		case models.HiveInfo:
			info.Hive = v
		case models.HbaseInfo:
			info.HBase = v
		case models.ZookeeperInfo:
			info.Zookeeper = v
		case models.OozieInfo:
			info.Oozie = v
		case models.LivyInfo:
			info.Livy = v
		}
	}

	if a.reportCount == math.MaxInt64 {
		a.reportCount = 0
	}
	a.reportCount += 1
	info.ReportCount = a.reportCount

	return info
}

// SetTopologyRevision atomicaly changes revision for report
func (a *Agent) SetTopologyRevision(topologyRevision int64) {
	a.topologyRevision.Store(topologyRevision)
}

// Run agent
func (a *Agent) Run(ctx context.Context) {
	ticker := time.NewTicker(a.cfg.Timeout)
	defer ticker.Stop()
	for {
		select {
		case <-ticker.C:
			a.sendReport(ctx)
		case <-ctx.Done():
			return
		}
	}
}

func (a *Agent) sendReport(ctx context.Context) {
	ctx, cancel := context.WithTimeout(ctx, a.cfg.Timeout)
	defer cancel()
	info := a.fetchInfo(ctx)
	nodesToDecommission, err := a.apiClient.Report(ctx, info)
	if err != nil {
		a.logger.Error(err.Error())
	}
	go a.decommissioner.Decommission(info, nodesToDecommission)
}
