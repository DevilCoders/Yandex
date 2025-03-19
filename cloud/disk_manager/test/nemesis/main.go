package main

import (
	"context"
	"fmt"
	"log"
	"math/rand"
	"os"
	"os/exec"
	"strings"
	"time"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
)

////////////////////////////////////////////////////////////////////////////////

func getRandomDuration(minSec uint32, maxSec uint32) time.Duration {
	rand.Seed(time.Now().UnixNano())
	x := int64(minSec) * 1000000
	y := int64(maxSec) * 1000000

	if y <= x {
		return time.Duration(minSec) * time.Second
	}

	return time.Duration(x+rand.Int63n(y-x)) * time.Microsecond
}

func createContext() context.Context {
	return logging.SetLogger(
		context.Background(),
		logging.CreateStderrLogger(logging.InfoLevel),
	)
}

func runIteration(ctx context.Context, cmdString string) error {
	logging.Info(ctx, "Running command: %v", cmdString)

	split := strings.Split(cmdString, " ")
	cmd := exec.CommandContext(ctx, split[0], split[1:]...)

	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr

	err := cmd.Start()
	if err != nil {
		return err
	}

	pid := cmd.Process.Pid

	logging.Info(ctx, "Waiting for process with PID: %v", pid)
	return cmd.Wait()
}

func waitIteration(
	ctx context.Context,
	cancel func(),
	errors chan error,
	minRestartPeriodSec uint32,
	maxRestartPeriodSec uint32,
) error {

	select {
	case <-time.After(getRandomDuration(minRestartPeriodSec, maxRestartPeriodSec)):
		logging.Info(ctx, "Cancel iteration")
		cancel()
		// Wait for iteration to stop.
		<-errors
		return nil
	case err := <-errors:
		logging.Error(ctx, "Received error during iteration: %v", err)
		return err
	}
}

func run(
	cmdString string,
	minRestartPeriodSec uint32,
	maxRestartPeriodSec uint32,
) error {

	cmdString = strings.TrimSpace(cmdString)
	if len(cmdString) == 0 {
		return fmt.Errorf("invalid command: %v", cmdString)
	}

	ctx := createContext()

	for {
		logging.Info(ctx, "Start iteration")

		iterationCtx, cancelIteration := context.WithCancel(ctx)
		defer cancelIteration()

		errors := make(chan error, 1)
		go func() {
			errors <- runIteration(iterationCtx, cmdString)
		}()

		err := waitIteration(
			iterationCtx,
			cancelIteration,
			errors,
			minRestartPeriodSec,
			maxRestartPeriodSec,
		)
		if err != nil {
			return err
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

func main() {
	var cmdString string
	var minRestartPeriodSec uint32
	var maxRestartPeriodSec uint32

	rootCmd := &cobra.Command{
		RunE: func(cmd *cobra.Command, args []string) error {
			return run(cmdString, minRestartPeriodSec, maxRestartPeriodSec)
		},
	}

	rootCmd.Flags().StringVar(&cmdString, "cmd", "", "command to execute")
	if err := rootCmd.MarkFlagRequired("cmd"); err != nil {
		log.Fatalf("Error setting flag cmd as required: %v", err)
	}

	rootCmd.Flags().Uint32Var(
		&minRestartPeriodSec,
		"min-restart-period-sec",
		5,
		"minimum time (in seconds) between two consecutive restarts",
	)

	rootCmd.Flags().Uint32Var(
		&maxRestartPeriodSec,
		"max-restart-period-sec",
		30,
		"maximum time (in seconds) between two consecutive restarts",
	)

	if err := rootCmd.Execute(); err != nil {
		log.Fatalf("Failed to execute: %v", err)
	}
}
