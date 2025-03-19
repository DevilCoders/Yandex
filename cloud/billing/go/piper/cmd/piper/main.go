package main

import (
	"log"
	"os"
	"time"

	"github.com/spf13/cobra"
	"golang.org/x/sync/errgroup"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/config"
)

var (
	cmd = &cobra.Command{
		Use:   os.Args[0],
		Short: "Run YCloud billing metrics processing tool",
		Run:   run,
	}

	showConfigCmd = cobra.Command{
		Use:   "show-config",
		Short: "Show result configuration builded for run for debug purposes",
		Run:   showConfig,
	}
)

var (
	configPaths      []string
	noConfigOverride bool
)

func init() {
	cmd.PersistentFlags().StringArrayVarP(&configPaths, "config", "c", nil, "Path to the config yaml file")
	_ = cmd.MarkPersistentFlagRequired("config")

	showConfigCmd.PersistentFlags().BoolVarP(&noConfigOverride, "no-override", "n", false, "Disable override config by db context")
	cmd.AddCommand(&showConfigCmd)
}

func main() {
	defer globalStop()
	log.SetOutput(os.Stderr)

	if err := cmd.Execute(); err != nil {
		os.Exit(1)
	}
}

func run(cmd *cobra.Command, args []string) {
	cont := getContainer(config.WithOverride)

	runCtx, stop := cont.GetRunContext()
	defer stop()

	stats, err := cont.GetHTTPStatusServer()
	if err != nil {
		cmd.PrintErrln(err)
		return
	}
	dumpers, err := cont.GetDumperServices()
	if err != nil {
		cmd.PrintErrln(err)
		return
	}

	resharders, err := cont.GetResharderServices()
	if err != nil {
		cmd.PrintErrln(err)
		return
	}

	wg, waitCtx := errgroup.WithContext(runCtx)
	wg.Go(stats.ListenAndServe)
	for _, d := range dumpers {
		wg.Go(d.Run)
	}
	for _, r := range resharders {
		wg.Go(r.Run)
	}

	<-waitCtx.Done()
	shutdownContainer(cont)
	if err := wg.Wait(); err != nil {
		cmd.PrintErrln(err)
	}
}

func showConfig(cmd *cobra.Command, args []string) {
	go func() {
		time.Sleep(time.Second * 10)
		globalStop()
	}()

	cont := getContainer(config.OverrideFlag(!noConfigOverride))
	defer shutdownContainer(cont)

	cfg, err := cont.GetApplicationConfig()
	if err != nil {
		cmd.PrintErrln(err)
		return
	}
	s, _ := yaml.Marshal(cfg)
	cmd.Println(string(s))
}
