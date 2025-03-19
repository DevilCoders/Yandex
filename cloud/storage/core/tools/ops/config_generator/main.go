package main

import (
	"context"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"path"
	"strings"
	"text/template"

	nbsProto "a.yandex-team.ru/cloud/blockstore/config"
	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	nfsProto "a.yandex-team.ru/cloud/filestore/config"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/configurator"
	kikimrProto "a.yandex-team.ru/cloud/storage/core/tools/common/go/configurator/kikimr-proto"

	"github.com/spf13/cobra"
	"gopkg.in/yaml.v2"
)

////////////////////////////////////////////////////////////////////////////////

type Options struct {
	ServicePath     string
	ArcadiaRootPath string
	Verbose         bool
}

////////////////////////////////////////////////////////////////////////////////

func getNbsConfigMap() configurator.ConfigMap {
	return configurator.ConfigMap{
		"nbs-server.txt":              {Proto: &nbsProto.TServerAppConfig{}, FileName: "server.txt"},
		"nbs-client.txt":              {Proto: &nbsProto.TClientAppConfig{}, FileName: "client.txt"},
		"nbs-features.txt":            {Proto: &nbsProto.TFeaturesConfig{}, FileName: "features.txt"},
		"nbs-storage.txt":             {Proto: &nbsProto.TStorageServiceConfig{}, FileName: "storage.txt"},
		"nbs-diag.txt":                {Proto: &nbsProto.TDiagnosticsConfig{}, FileName: "diagnostics.txt"},
		"nbs-stats-upload.txt":        {Proto: &nbsProto.TYdbStatsConfig{}, FileName: "ydbstats.txt"},
		"nbs-discovery.txt":           {Proto: &nbsProto.TDiscoveryServiceConfig{}, FileName: "discovery.txt"},
		"nbs-logbroker.txt":           {Proto: &nbsProto.TLogbrokerConfig{}, FileName: "logbroker.txt"},
		"nbs-notify.txt":              {Proto: &nbsProto.TNotifyConfig{}, FileName: "notify.txt"},
		"nbs-disk-registry-proxy.txt": {Proto: &nbsProto.TDiskRegistryProxyConfig{}, FileName: "disk-registry.txt"},
		"nbs-http-proxy.txt":          {Proto: &nbsProto.THttpProxyConfig{}, FileName: "http-proxy.txt"},
		"nbs-local-storage.txt":       {Proto: &nbsProto.TLocalStorageConfig{}, FileName: "local-storage.txt"},

		// for kikimr initializer configs used custom protobuf files
		// from cloud/storage/core/tools/common/go/configurator/kikimr-proto
		// with a minimum set of parameters to avoid dependencies
		"nbs-auth.txt": {Proto: &kikimrProto.TAuthConfig{}, FileName: "auth.txt"},
		"nbs-ic.txt":   {Proto: &kikimrProto.TInterconnectConfig{}, FileName: "ic.txt"},
		"nbs-log.txt":  {Proto: &kikimrProto.TLogConfig{}, FileName: "log.txt"},
		"nbs-sys.txt":  {Proto: &kikimrProto.TActorSystemConfig{}, FileName: "sys.txt"},
	}
}

func getNfsConfigMap() configurator.ConfigMap {
	return configurator.ConfigMap{
		"nfs-server.txt":     {Proto: &nfsProto.TServerAppConfig{}, FileName: "server.txt"},
		"nfs-storage.txt":    {Proto: &nfsProto.TStorageConfig{}, FileName: "storage.txt"},
		"nfs-diag.txt":       {Proto: &nfsProto.TDiagnosticsConfig{}, FileName: "diagnostics.txt"},
		"nfs-http-proxy.txt": {Proto: &nfsProto.THttpProxyConfig{}, FileName: "http-proxy.txt"},
		"nfs-vhost.txt":      {Proto: &nfsProto.TVhostAppConfig{}, FileName: "vhost.txt"},

		// for kikimr initializer configs used custom protobuf files
		// from cloud/storage/core/tools/common/go/configurator/kikimr-proto
		// with a minimum set of parameters to avoid dependencies
		"nfs-auth.txt": {Proto: &kikimrProto.TAuthConfig{}, FileName: "auth.txt"},
		"nfs-ic.txt":   {Proto: &kikimrProto.TInterconnectConfig{}, FileName: "ic.txt"},
		"nfs-log.txt":  {Proto: &kikimrProto.TLogConfig{}, FileName: "log.txt"},
		"nfs-sys.txt":  {Proto: &kikimrProto.TActorSystemConfig{}, FileName: "sys.txt"},
	}
}

func getDiskManagerConfigMap() configurator.ConfigMap {
	return configurator.ConfigMap{}
}

func getConfigMap(serviceName string) (configurator.ConfigMap, error) {
	switch serviceName {
	case "nbs":
		return getNbsConfigMap(), nil
	case "nfs":
		return getNfsConfigMap(), nil
	case "disk-manager":
		return getDiskManagerConfigMap(), nil
	default:
		return nil, fmt.Errorf("unknown service: %v", serviceName)
	}
}

func loadServiceConfig(configPath string) (*configurator.ServiceSpec, error) {
	configTmpl, err := ioutil.ReadFile(path.Join(configPath, "spec.yaml"))
	if err != nil {
		return nil, fmt.Errorf("can't read service config: %w", err)
	}

	tmpl, err := template.New("config").Parse(string(configTmpl))
	if err != nil {
		return nil, fmt.Errorf("can't parse config: %w", err)
	}

	var configYAML strings.Builder

	err = tmpl.Execute(&configYAML, nil)
	if err != nil {
		return nil, fmt.Errorf("can't execute config: %w", err)
	}

	config := &configurator.ServiceSpec{}
	if err := yaml.Unmarshal([]byte(configYAML.String()), config); err != nil {
		return nil, fmt.Errorf("can't unmarshal config: %w", err)
	}

	return config, nil
}

func run(opts *Options, ctx context.Context) error {
	logLevel := nbs.LOG_INFO
	if opts.Verbose {
		logLevel = nbs.LOG_DEBUG
	}

	logger := nbs.NewLog(
		log.New(
			os.Stdout,
			"",
			log.Ltime,
		),
		logLevel,
	)

	var config, err = loadServiceConfig(opts.ServicePath)
	if err != nil {
		return fmt.Errorf("can't load config: %w", err)
	}

	configMap, err := getConfigMap(config.ServiceName)
	if err != nil {
		return err
	}

	return configurator.NewConfigGenerator(
		logger,
		configurator.GeneratorSpec{
			ServiceSpec:   *config,
			ConfigMap:     configMap,
			ArcadiaPath:   opts.ArcadiaRootPath,
			OverridesPath: opts.ServicePath}).Generate(ctx)
}

func main() {
	var opts Options

	var rootCmd = &cobra.Command{
		Use:   "config-generator",
		Short: "Config generator",
		Run: func(cmd *cobra.Command, args []string) {
			ctx, cancelCtx := context.WithCancel(context.Background())
			defer cancelCtx()

			if err := run(&opts, ctx); err != nil {
				log.Fatalf("Error: %v", err)
			}
		},
	}

	rootCmd.Flags().StringVarP(
		&opts.ServicePath,
		"service-path",
		"s",
		"",
		"path to folder with service spec",
	)

	rootCmd.Flags().StringVarP(
		&opts.ArcadiaRootPath,
		"arcadia-root-path",
		"a",
		"",
		"path to arcadia root",
	)

	rootCmd.Flags().BoolVarP(&opts.Verbose, "verbose", "v", false, "verbose mode")

	requiredFlags := []string{
		"service-path", "arcadia-root-path",
	}

	for _, flag := range requiredFlags {
		if err := rootCmd.MarkFlagRequired(flag); err != nil {
			log.Fatalf("can't mark flag %v as required: %v", flag, err)
		}
	}

	if err := rootCmd.Execute(); err != nil {
		log.Fatalf("can't execute root command: %v", err)
	}
}
