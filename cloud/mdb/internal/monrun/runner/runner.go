package runner

import (
	"context"
	"fmt"
	"os"
	"time"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/monrun"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/log/zap"
)

var (
	checkTimeout time.Duration
	printDebug   bool
)

func init() {
	monitoringFlags := pflag.NewFlagSet("Monitoring", pflag.ExitOnError)
	monitoringFlags.BoolVar(&printDebug, "debug", false, "Print log output")
	monitoringFlags.DurationVar(&checkTimeout, "timeout", time.Minute, "Timeout for a check")
	pflag.CommandLine.AddFlagSet(monitoringFlags)
}

type CheckHandler func(context.Context, log.Logger) monrun.Result

func newLogger() log.Logger {
	if printDebug {
		cliLogger, err := zap.New(zap.KVConfig(log.DebugLevel))
		if err == nil {
			return cliLogger
		}
		fmt.Printf("Unable to create logger. Unsupported level: %s", err)
		os.Exit(1)
	}
	return &nop.Logger{}
}

// RunCheck run handler and prints its result
//	caller should call pflags.Parse()
func RunCheck(handler CheckHandler) {
	ctx := context.Background()
	ctx, cancel := context.WithTimeout(ctx, checkTimeout)
	defer cancel()

	logger := newLogger()

	result := handler(ctx, logger)
	fmt.Println(result)
}
