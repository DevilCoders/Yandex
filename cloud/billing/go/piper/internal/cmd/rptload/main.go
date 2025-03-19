package main

import (
	"context"
	"os"
	"os/signal"
	"syscall"

	"github.com/spf13/cobra"
)

func main() {
	mainCtx, globalStop := context.WithCancel(context.Background())
	defer globalStop()

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		select {
		case <-sigChan:
			globalStop()
		case <-mainCtx.Done():
		}
	}()

	rootCmd := &cobra.Command{
		Use: "rptload",
	}
	rootCmd.AddCommand(NewCmdMetricsToYdb(mainCtx).BuildCobraCommand())
	rootCmd.AddCommand(NewCmdMetricsToFile(mainCtx).BuildCobraCommand())
	rootCmd.AddCommand(NewCmdReportToYdb(mainCtx).BuildCobraCommand())

	if err := rootCmd.Execute(); err != nil {
		os.Exit(1)
	}
}
