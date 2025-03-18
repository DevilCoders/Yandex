package cmd

import (
	"errors"
	"fmt"
	"os"

	"github.com/sirupsen/logrus"
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cdn/m3/cmd/config"
	"a.yandex-team.ru/cdn/m3/core"
	"a.yandex-team.ru/cdn/m3/events"
	"a.yandex-team.ru/cdn/m3/monitor"
	"a.yandex-team.ru/cdn/m3/utils"
)

var (
	// some global variables
	configFile string
	debug      bool
	log        string

	// Error on initialization
	InitError error

	g config.CmdGlobal

	rootCmd = &cobra.Command{
		SilenceUsage: true,
		Use:          "m3",
		Short:        "m3 service application",
		Long: fmt.Sprintf(`
m3 is a controller to manage (1) mpls tunnels (2) mpls transits. The
following scheme is used: (a) m3 starts bgp session to specified bgp
peers (b) m3 waits for bgp messages and processes them as they arrive
(c) m3 has bgp state paths received (d) periodically it syncronizes
bgp knowledge to network objects: links, routes and rules. It could
control target system at host or in lxd container, [1]

[1] https://wiki.yandex-team.ru/tt/m3/
`),
		PersistentPreRun: func(cmd *cobra.Command, args []string) {
			if InitError != nil {
				os.Exit(1)
			}

			// Updaing log options before each command
			// as switches could change some log options:
			// level, stdout and etc
			var Overrides config.ConfigOverrides
			Overrides.Debug = debug
			Overrides.Log = log
			if g.Log != nil {
				g.Log.UpdateOverrides(Overrides)
			}
		},
	}
)

func Execute() error {
	return rootCmd.Execute()
}

func init() {
	cobra.OnInitialize(InitConfig)

	g = config.CmdGlobal{Cmd: rootCmd, Opts: &config.ConfYaml{}}

	var examples = []string{
		`  a) main function: syncing data from bgp session and
     generating networks objects, syncing bgp sessions with networking ip
     route objects

     m3 mgmt sync --debug  --timeout=20 [--dry-run]`,
		`  b) flushing all network objects on target system, with
     dry-run flushing all operations are prepared but no actions are done

     m3 mgmt flush --debug  [--dry-run]`,
	}

	for _, v := range examples {
		if len(rootCmd.Example) > 0 {
			rootCmd.Example = fmt.Sprintf("%s\n\n%s", rootCmd.Example, v)
		} else {
			rootCmd.Example = v
		}
	}

	// global command line switches, could override
	// configuration variables
	rootCmd.PersistentFlags().StringVarP(&configFile, "config file", "C", "",
		"configuration file")
	rootCmd.PersistentFlags().BoolVarP(&debug, "debug", "d",
		false, "debug output, default: 'false'")
	rootCmd.PersistentFlags().StringVarP(&log, "log", "", "",
		"log file, default: 'stdout'")

	socket := "/var/run/m3.socket"

	// Events intgeration, configuration?
	g.Events = events.CreateEvents("/usr/bin/m3",
		logrus.New(), rootCmd, socket)

	// Monitoring integration and configuration
	g.Monitor = monitor.CreateMonitor("/usr/bin/m3",
		logrus.New(), rootCmd,
		socket, "", "", "monitor")

	// Monitoring checks intergation (for client
	// side checks, for server side intergration is
	// done as server started
	C, _ := core.CreateCore(&g)
	C.CoreMonitor(g.Monitor)

	// "config" shows current config and
	// runtime variables set
	configCmd := cmdConfig{g: &g}
	rootCmd.AddCommand(configCmd.Command())

	// "version" shows version of binary run
	versionCmd := cmdVersion{g: &g}
	rootCmd.AddCommand(versionCmd.Command())

	// server sub-command
	serverCmd := cmdServer{g: &g}
	serverCmd.core = C
	rootCmd.AddCommand(serverCmd.Command())

	// mgmt implements bgp session handling from
	// command line and some tasks
	mgmtCmd := cmdMgmt{g: &g}
	mgmtCmd.core = C
	rootCmd.AddCommand(mgmtCmd.Command())

	statusCmd := cmdStatus{g: &g}
	statusCmd.core = C
	rootCmd.AddCommand(statusCmd.Command())

	// "completion" for bash
	var completionCmd = &cobra.Command{
		Use:    "completion",
		Hidden: true,
		Short:  "Generates bash completion scripts",
		Long: `To load completion run

. <(bitbucket completion)

To configure your bash shell to load completions for each session add to your bashrc

# ~/.bashrc or ~/.profile
. <(bitbucket completion)
`,
		Run: func(cmd *cobra.Command, args []string) {
			rootCmd.GenBashCompletion(os.Stdout)
		},
	}

	rootCmd.AddCommand(completionCmd)

	return
}

func EmergencyLog(err string, Overrides config.ConfigOverrides) {
	// Initialization for logger if no configuraion given
	Log := logrus.New()
	Log.Formatter = &logrus.TextFormatter{
		TimestampFormat: "2006/01/02 - 15:04:05.000",
		FullTimestamp:   true,
	}

	level, _ := logrus.ParseLevel("info")
	if Overrides.Debug {
		level, _ = logrus.ParseLevel("debug")
	}

	Log.Level = level
	Log.Out = os.Stdout
	Log.Error(err)
}

func InitConfig() {
	var err error
	var Overrides config.ConfigOverrides
	Overrides.Debug = debug
	Overrides.Log = log

	*g.Opts, InitError = config.LoadConf(configFile, Overrides)
	g.Opts.Runtime.Version = version

	if InitError != nil {
		// Initialization for logger if no configuraion given
		EmergencyLog(fmt.Sprintf("%s configuration file load failed, err:'%s'",
			utils.GetGPid(), InitError), Overrides)
		os.Exit(1)
		return
	}

	if g.Log, err = config.CreateLog(g.Opts); err != nil {
		InitError := errors.New(fmt.Sprintf("logfile initialization error, err:'%s'", err))
		EmergencyLog(fmt.Sprintf("%s %s", utils.GetGPid(), InitError), Overrides)
		os.Exit(1)
		return
	}

	// Setting additional configuration parameters
	g.Monitor.SolomonMetrics.Enabled = g.Opts.Solomon.Enabled
	g.Monitor.SolomonMetrics.Endpoint = g.Opts.Solomon.Endpoint
	g.Monitor.SolomonMetrics.Project = g.Opts.Solomon.Project
	g.Monitor.SolomonMetrics.Token = g.Opts.Solomon.Token
	g.Monitor.SolomonMetrics.Cluster = g.Opts.Solomon.Cluster
	g.Monitor.SolomonMetrics.Host = g.Opts.Runtime.Hostname

	g.Monitor.GraphiteMetrics.Enabled = false

	g.Monitor.SolomonMetrics.Service = "hx"
	g.Monitor.SolomonMetrics.Location = "unknown"
	g.Monitor.SolomonMetrics.Role = config.PROGRAM_NAME

	if g.Opts.Juggler.Enabled {
		g.Monitor.JugglerMetrics.Host = g.Opts.Runtime.Hostname
		g.Monitor.JugglerMetrics.Service = config.PROGRAM_NAME
		g.Monitor.JugglerMetrics.Url = g.Opts.Juggler.Url
	}

	g.Monitor.SetLog(g.Log.Log)

	socket := g.Opts.M3.Socket
	g.CreateSingletons(socket)

}
