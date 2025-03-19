package main

import (
	"github.com/spf13/cobra"

	"a.yandex-team.ru/admins/zk"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const defaultConfigPattern = "/etc/distributed-flock*.json"

var logger log.Logger
var loggerConfig = zap.ConsoleConfig(log.FatalLevel)

var (
	flagDebug     bool
	flagWatch     bool
	flagConfig    string
	flagZkServers string
	flagRecursive bool
	flagVersion   int32
	zkClient      zk.Zk
)

func init() {
	loggerConfig.OutputPaths = []string{"stderr"}
	logger = zap.Must(loggerConfig)

	rootCmd.AddCommand(
		cmdExists(&zkClient),
		cmdStat(&zkClient),
		cmdGet(&zkClient),
		cmdCreate(&zkClient),
		cmdSet(&zkClient),
		cmdDelete(&zkClient),
		cmdChildren(&zkClient),
		cmdDump(&zkClient),
		cmdRestore(&zkClient),
	)

	rootCmd.PersistentFlags().BoolVarP(&flagDebug, "debug", "d", false, "enable debug logs")
	rootCmd.PersistentFlags().StringVarP(&flagConfig, "config", "c", "", "config file (default: "+defaultConfigPattern+")")
	rootCmd.PersistentFlags().StringVarP(&flagZkServers, "servers", "s", "",
		"precidence of the servers resolving: --servers or env['ZOOKEEPER_SERVERS'] or "+defaultConfigPattern+" or localhost")
}

var rootCmd = &cobra.Command{
	Use:               "zk [command] [options]",
	Short:             "Go zk client",
	PersistentPreRun:  getClient,
	PersistentPostRun: func(_ *cobra.Command, _ []string) { zkClient.Close() },
}

func main() {
	err := rootCmd.Execute()
	logger.Debugf("Command ended with err=%v", err)
}
