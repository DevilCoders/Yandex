package main

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"path"
	"path/filepath"
	"sort"
	"strings"
	"time"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/admins/zk"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func fixPath(path string) string {
	path = strings.Trim(path, "/")
	return "/" + path
}

func must(err error, context ...string) {
	if err != nil {
		contextStr := ""
		if len(context) > 0 {
			contextStr = fmt.Sprintf(", %v", context)
		}
		logger.Fatalf("Fatal error: %v%s", err, contextStr)
	}
}

func getClient(_ *cobra.Command, _ []string) {
	level := log.FatalLevel
	if flagDebug {
		loggerConfig.Level.SetLevel(zap.ZapifyLevel(log.TraceLevel))
		level = log.TraceLevel
	}

	svs := resolveZkServers()
	cfg := &zk.Config{
		Servers:          svs,
		Timeout:          time.Second,
		MaxRetryAttempts: 3,
	}
	var zkClientRef = &zkClient
	*zkClientRef = *zk.New(cfg, logger, level)
}

func formatTime(millis int64) string {
	t := time.Unix(0, millis*1000000)
	return t.Format(time.RFC3339)
}
func processChildrenRecursivelyPostorder(ctx context.Context, client zk.Client, path string, callback func(string)) {
	postorderCallback := func(path string) bool {
		callback(path)
		return false
	}
	processChildrenRecursively(ctx, client, path, true, postorderCallback)
}

func processChildrenRecursivelyPreorder(ctx context.Context, client zk.Client, path string, callback func(string) bool) {
	processChildrenRecursively(ctx, client, path, false, callback)
}

func processChildrenRecursively(ctx context.Context, client zk.Client, nodePath string, postorder bool, callback func(string) bool) {
	logger.Debugf("Process node: %s", nodePath)
	children, _, err := client.Children(ctx, nodePath)
	if err != nil {
		if err == zk.ErrNoNode {
			return
		}
		must(err)
	}

	sort.Strings(children)
	for _, child := range children {
		childrenPath := path.Join(nodePath, child)
		skip := false
		if !postorder {
			skip = callback(childrenPath)
		}
		if !skip {
			processChildrenRecursively(ctx, client, childrenPath, postorder, callback)
		}
		if postorder {
			_ = callback(childrenPath)
		}
	}
}

type distributedZkFlockConfig struct {
	Host []string `json:"host"`
}

func parseZkConnString(connStr string) []string {
	var zkServers []string
	for _, srv := range strings.Split(connStr, ",") {
		srv = strings.TrimSpace(srv)
		if srv != "" {
			zkServers = append(zkServers, srv)
		}
	}
	if len(zkServers) == 0 {
		logger.Fatalf("Failed to parse zk connection string %s", connStr)
	}
	return zkServers
}

func resolveZkServers() []string {
	var defaultServers = []string{"127.0.0.1:2181"}
	if flagZkServers != "" {
		return parseZkConnString(flagZkServers)
	}

	envSrv := os.Getenv("ZOOKEEPER_SERVERS")
	if envSrv != "" {
		return parseZkConnString(envSrv)
	}

	// if env var empty then use config
	// if pattern not set try defaultConfigPattern
	if flagConfig == "" {
		cfgs, err := filepath.Glob(defaultConfigPattern)
		if err != nil {
			logger.Fatalf("Wrong glob pattern :%s, %v", defaultServers, err)
		}
		if len(cfgs) == 0 {
			logger.Infof("No config files found for pattern %s. Will use default servers: %s", defaultConfigPattern, defaultServers)
			return defaultServers
		}

		flagConfig = cfgs[0]
	}

	logger.Infof("Reading config file: %s", flagConfig)
	rawConfig, err := ioutil.ReadFile(flagConfig)
	if err != nil {
		logger.Fatalf("Failed to read config: %v", err)
	}
	var hosts distributedZkFlockConfig
	err = json.Unmarshal(rawConfig, &hosts)
	if err != nil || len(hosts.Host) == 0 {
		if err == nil {
			err = fmt.Errorf("empty 'host' list: %s", flagConfig)
		}
		logger.Fatalf("Failed to parse config: %v", err)
	}
	return hosts.Host
}
