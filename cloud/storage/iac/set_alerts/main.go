package main

import (
	"context"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"strings"
	"text/template"

	"github.com/spf13/cobra"
	"gopkg.in/yaml.v2"

	secutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/secrets"
	solomonadmin "a.yandex-team.ru/cloud/storage/core/tools/common/go/solomon/admin"
	solomon "a.yandex-team.ru/cloud/storage/core/tools/common/go/solomon/sdk"
)

////////////////////////////////////////////////////////////////////////////////

type options struct {
	solomonadmin.Options

	Secrets       string
	ServiceConfig string
}

////////////////////////////////////////////////////////////////////////////////

func loadServiceConfig(path string) (*solomonadmin.ServiceConfig, error) {
	configTmpl, err := ioutil.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("can't read service config: %w", err)
	}

	funcs := template.FuncMap{
		"seq": func(args ...string) []string {
			var seq []string
			return append(seq, args...)
		},
		"ToUpper": strings.ToUpper,
	}

	tmpl, err := template.New("config").Funcs(funcs).Parse(string(configTmpl))
	if err != nil {
		return nil, fmt.Errorf("can't parse service config: %w", err)
	}

	var configYAML strings.Builder

	err = tmpl.Execute(&configYAML, nil)
	if err != nil {
		return nil, fmt.Errorf("can't execute service config: %w", err)
	}

	config := &solomonadmin.ServiceConfig{}
	if err := yaml.Unmarshal([]byte(configYAML.String()), config); err != nil {
		return nil, fmt.Errorf("can't unmarshal service config: %w", err)
	}

	return config, nil
}

////////////////////////////////////////////////////////////////////////////////

func getToken(config *solomonadmin.ServiceConfig, secrets *secutil.Secrets) string {
	switch config.SolomonAuthType {
	case solomon.IAM:
		iamToken := os.Getenv("IAM_TOKEN")
		if len(iamToken) != 0 {
			return iamToken
		}
		return secrets.SolomonIamToken
	case solomon.OAuth:
		return secrets.SolomonOAuthToken
	}

	return secrets.SolomonOAuthToken
}

func run(ctx context.Context, opts *options) error {
	config, err := loadServiceConfig(opts.ServiceConfig)
	if err != nil {
		return err
	}

	err = solomonadmin.PrepareClusterConfigs(config)
	if err != nil {
		return err
	}

	secrets, err := secutil.LoadFromFile(opts.Secrets)
	if err != nil {
		return err
	}

	solomonURL := config.SolomonURL
	if len(solomonURL) == 0 {
		solomonURL = solomon.SolomonURL
	}

	solomonClient := solomon.NewClient(
		config.SolomonProjectID,
		solomonURL,
		getToken(config, secrets),
		config.SolomonAuthType,
	)

	if len(opts.Options.ChannelsPath) != 0 {
		r := solomonadmin.NewChannelManager(
			&opts.Options,
			config,
			solomonClient,
		)

		err = r.Run(ctx)
		if err != nil {
			return err
		}
	}

	if len(opts.Options.ClustersPath) != 0 {
		r := solomonadmin.NewClusterManager(
			&opts.Options,
			config,
			solomonClient,
		)

		err = r.Run(ctx)
		if err != nil {
			return err
		}
	}

	if len(opts.Options.ServicesPath) != 0 {
		r := solomonadmin.NewServicesManager(
			&opts.Options,
			config,
			solomonClient,
		)

		err = r.Run(ctx)
		if err != nil {
			return err
		}
	}

	if len(opts.Options.ShardsPath) != 0 {
		r := solomonadmin.NewShardManager(
			&opts.Options,
			config,
			solomonClient,
		)

		err = r.Run(ctx)
		if err != nil {
			return err
		}
	}

	if len(opts.Options.AlertsPath) != 0 {
		r := solomonadmin.NewAlertManager(
			&opts.Options,
			config,
			solomonClient,
		)

		err = r.Run(ctx)
		if err != nil {
			return err
		}
	}

	if len(opts.Options.DashboardsPath) != 0 {
		r := solomonadmin.NewDashboardManager(
			&opts.Options,
			config,
			solomonClient,
		)

		err = r.Run(ctx)
		if err != nil {
			return err
		}
	}

	if len(opts.Options.GraphsPath) != 0 {
		r := solomonadmin.NewGraphManager(
			&opts.Options,
			config,
			solomonClient,
		)

		err = r.Run(ctx)
		if err != nil {
			return err
		}
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func main() {
	var opts options

	var rootCmd = &cobra.Command{
		Use:   "set-alerts",
		Short: "Set alert tool",
		Run: func(cmd *cobra.Command, args []string) {
			ctx, cancelCtx := context.WithCancel(context.Background())
			defer cancelCtx()

			if err := run(ctx, &opts); err != nil {
				log.Fatalf("Error: %v", err)
			}
		},
	}

	rootCmd.Flags().StringVarP(
		&opts.ServiceConfig,
		"service-config",
		"c",
		"",
		"service config YAML file path",
	)

	rootCmd.Flags().StringVarP(
		&opts.AlertsPath,
		"alerts-path",
		"f",
		"",
		"path to file/folder with alert YAML configs",
	)

	rootCmd.Flags().StringVarP(
		&opts.ChannelsPath,
		"channels-path",
		"",
		"",
		"path to file/folder with channel YAML configs",
	)

	rootCmd.Flags().StringVarP(
		&opts.ClustersPath,
		"clusters-path",
		"",
		"",
		"path to file/folder with cluster YAML configs",
	)

	rootCmd.Flags().StringVarP(
		&opts.ServicesPath,
		"services-path",
		"",
		"",
		"path to file/folder with service YAML configs",
	)

	rootCmd.Flags().StringVarP(
		&opts.ShardsPath,
		"shards-path",
		"",
		"",
		"path to file/folder with shard YAML configs",
	)

	rootCmd.Flags().StringVarP(
		&opts.DashboardsPath,
		"dashboards-path",
		"",
		"",
		"path to file/folder with dashboard YAML configs",
	)

	rootCmd.Flags().StringVarP(
		&opts.GraphsPath,
		"graphs-path",
		"",
		"",
		"path to file/folder with graph YAML configs",
	)

	rootCmd.Flags().StringVarP(
		&opts.Cluster,
		"cluster",
		"",
		"",
		"apply changes only for cluster",
	)

	rootCmd.Flags().BoolVarP(
		&opts.Remove,
		"remove",
		"r",
		false,
		"remove untracked alerts",
	)

	rootCmd.Flags().BoolVarP(
		&opts.ApplyChanges,
		"apply",
		"a",
		false,
		"apply changes",
	)

	rootCmd.Flags().BoolVarP(
		&opts.ShowDiff,
		"diff",
		"d",
		false,
		"show diff",
	)

	rootCmd.Flags().StringVarP(
		&opts.Secrets,
		"secrets",
		"s",
		func() string {
			path, _ := secutil.GetDefaultSecretsPath()
			return path
		}(),
		"secrets JSON file path",
	)

	requiredFlags := []string{
		"service-config",
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
