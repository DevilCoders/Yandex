package common

import (
	"context"
	"fmt"
	"io"
	"net/http"
	"os"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/app/signals"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/flags"
	"a.yandex-team.ru/cloud/mdb/internal/pretty"
	"a.yandex-team.ru/cloud/mdb/internal/tilde"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const (
	flagNameCACerts        = "capath"
	flagNameOutput         = "output"
	flagNameGenerateConfig = "genconfig"
	flagNameDownloadCA     = "downloadca"
	flagNameDeployment     = "deployment"

	caFileURL = "https://crls.yandex.ru/allCAs.pem"
)

var (
	flagValueCACerts    string
	flagValueOutput     string
	FlagValueDeployment string
)

func defaultCAPath() string {
	return config.DefaultConfig().CAPath
}

// Commander implements cli root command and supporting functionality
type Commander struct {
	ctx     context.Context
	env     *cli.Env
	rootCmd *cli.Command
}

// New constructs generic mdb-cli Commander
func New(name string, rootCmd *cli.Command) *Commander {
	// Fill root cmd with common flags and values
	rootCmd.Cmd.PersistentFlags().StringVar(
		&flagValueCACerts,
		flagNameCACerts,
		"",
		"path to CA file",
	)
	rootCmd.Cmd.PersistentFlags().StringVar(
		&flagValueOutput,
		flagNameOutput,
		"",
		"output format (yaml, json)",
	)
	rootCmd.Cmd.PersistentFlags().StringVarP(
		&FlagValueDeployment,
		flagNameDeployment,
		"d",
		"",
		"use specified deployment",
	)

	rootCmd.Cmd.Flags().Bool(
		flagNameGenerateConfig,
		false,
		"generate default config at default",
	)
	rootCmd.Cmd.Flags().Bool(
		flagNameDownloadCA,
		false,
		fmt.Sprintf("download CA file to '%s'", defaultCAPath()),
	)
	ctx := signals.WithCancelOnSignal(context.Background())
	cmdr := &Commander{
		ctx:     ctx,
		rootCmd: rootCmd,
	}
	rootCmd.Run = cmdr.run
	rootCmd.Cmd.PersistentPreRunE = cmdr.persistentPreRunE
	cmdr.env = cli.NewWithOptions(cmdr.ctx, cmdr.rootCmd,
		cli.WithFlagLogShowAll(),
		cli.WithFlagDryRun(),
		cli.WithCustomConfig(config.FormatConfigFilePath(config.DefaultConfigPath, name)))
	_ = rootCmd.Cmd.RegisterFlagCompletionFunc(
		flagNameDeployment,
		func(cmd *cobra.Command, args []string, toComplete string) ([]string, cobra.ShellCompDirective) {
			cfg := config.FromEnv(cmdr.env)
			return cfg.DeploymentNames(), cobra.ShellCompDirectiveNoFileComp
		},
	)
	return cmdr
}

// Execute runs cli commands
func (c *Commander) Execute() {
	// Ignore errors because they will be report by cobra (we do not silence them)

	_ = c.rootCmd.Cmd.Execute()
}

func (c *Commander) bindFlagToConfigValueString(cmd *cobra.Command, name string, value *string) {
	flag := cmd.Flag(name)
	if flag.Changed {
		*value = flag.Value.String()
	}
}

func (c *Commander) run(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	if c.downloadCA(cmd, env.Logger) {
		return
	}

	if c.generateConfig(cmd, env.Logger) {
		return
	}

	// TODO wrap RunE too so we can return errors
	_ = cmd.Help()
}

func (c *Commander) generateConfig(cmd *cobra.Command, logger log.Logger) bool {
	flag := cmd.Flag(flagNameGenerateConfig)
	if flag == nil || !flag.Changed {
		return false
	}

	path, err := tilde.Expand(config.DefaultConfigPath)
	if err != nil {
		panic(err)
	}

	filepath := config.FormatConfigFilePath(path, cmd.Use)
	logger.Infof("Generating config at %q...", filepath)

	if err = config.WriteConfig(config.DefaultConfig(), filepath); err != nil {
		logger.Fatalf("failed to generate to generate config at %q: %s", filepath, err)
	}

	logger.Infof("Config generated at %q", filepath)

	logger.Infof("Downloading CA file from '%s' to '%s'...", caFileURL, defaultCAPath())

	if err := downloadFile(caFileURL, defaultCAPath()); err != nil {
		logger.Errorf("Failed to download CA file to '%s': %s", defaultCAPath(), err)
		logger.Errorf(
			"You can download it manually and put at '%s' or try running 'mdb-support --%s'.",
			defaultCAPath(),
			flagNameDownloadCA,
		)
	} else {
		logger.Infof("CA file downloaded.")
	}

	return true
}

func (c *Commander) downloadCA(cmd *cobra.Command, logger log.Logger) bool {
	flag := cmd.Flag(flagNameDownloadCA)
	if flag == nil || !flag.Changed {
		return false
	}

	logger.Infof("Downloading CA file from '%s' to '%s'...", caFileURL, defaultCAPath())

	if err := downloadFile(caFileURL, defaultCAPath()); err != nil {
		logger.Fatalf("Failed to download CA file to '%s': %s", defaultCAPath(), err)
	}

	logger.Infof("CA file downloaded.")
	return true
}

func downloadFile(url, path string) error {
	// #nosec G107
	resp, err := http.Get(url)
	if err != nil {
		return err
	}
	defer func() {
		if err = resp.Body.Close(); err != nil {
			panic(fmt.Sprintf("Error closing response body: %s", err))
		}
	}()

	path, err = tilde.Expand(path)
	if err != nil {
		return err
	}

	f, err := os.OpenFile(path, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0600)
	if err != nil {
		return err
	}

	_, err = io.Copy(f, resp.Body)
	return err
}

func (c *Commander) persistentPreRunE(cmd *cobra.Command, args []string) error {
	cfg := config.DefaultConfig()
	var err error

	// Load config file only if we are executing a non-root command
	if cmd != c.rootCmd.Cmd {
		cfg, err = config.LoadConfig(c.env.GetConfigPath(), c.env.Logger)
		if err != nil {
			return err
		}

		// Recreate logger if needed
		_, logset, _ := flags.FlagLogLevel()
		if !logset {
			logger, err := zap.New(zap.CLIConfig(cfg.LogLevel))
			if err != nil {
				return err
			}
			c.env.Logger = logger
		}

		// use specified deployment if set
		if FlagValueDeployment != "" {
			if err = config.IsValidDeployment(cfg, FlagValueDeployment); err != nil {
				return err
			}

			cfg.SelectedDeployment = FlagValueDeployment
		}

		c.bindFlagToConfigValueString(cmd, flagNameCACerts, &cfg.CAPath)
		flag := cmd.Flag(flagNameOutput)
		if flag.Changed {
			cfg.Output = pretty.Format(flag.Value.String())
		}
	}

	c.env.Config = &cfg
	c.env.OutMarshaller = pretty.New(cfg.Output)
	return nil
}
