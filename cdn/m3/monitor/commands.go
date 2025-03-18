package monitor

import (
	"errors"
	"fmt"

	"github.com/sirupsen/logrus"
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cdn/m3/utils"
)

// Juggler monitoring intergration via commands:
// <cmd> monitor "check-id" --juggler|--juggler-config|--graphite

type MonitorSwitches struct {
	// Showing graphite metrics associated with monitoring
	// checks, dry-run flag shows not to send data into
	// graphite/redis, just show them as they are generated
	ShowGraphite bool
	Dryrun       bool

	// Showing juggler check
	ShowJuggler bool

	// Textual checks representations, could be splitted for
	// a list of objects as a list of rows (crlf flag)
	ShowStatus bool
	Crlf       bool

	// Showing juggler configuration for a check id given
	GenerateJugglerConfig bool
}

// Check-id is <cmd>-<uniq-id>, e.g. d2-dns-soa, d2-dns-bind, ...
// d2 ia a name of juggler section, dns-soa, dns-bind are named
// for juggler ids
type cmdMonitor struct {

	// A core object for checks with shared memories
	// histories and so on
	monitor *Monitor

	switches MonitorSwitches

	// Actual command string, it could be used
	// for intergration if "monitor" command is used
	// already
	command string

	debug bool
}

func (c *cmdMonitor) Command() *cobra.Command {

	cmd := &cobra.Command{}

	if len(c.command) == 0 {
		c.command = "monitor"
	}
	cmd.Use = fmt.Sprintf("%s --- ", c.command)

	cmd.Short = "Listing and executing monitoring checks and their results"
	cmd.Long = `
Monitor module is used to integrate an application with
juggler checks mechanism: getting application checks results,
generating a juggler configurations, sending graphite metrics
associated with juggler checks (e.g. a number of zones that
are differenent in d2-dns-soa check)
`

	var examples = []string{
		`  a) showing juggler configuration for d2-dns-soa
      $program monitor list d2-dns-soa --juggler-config`,
		`  b) making a juggler check run with debug (not all
      checks could be run in the mode, as a client)
      $program monitor run d2-dns-soa --debug`,
		`  c) making a juggler check server (cached) run with debug
      $program monitor list d2-dns-soa --immediately --debug`,
	}

	for _, v := range examples {
		if len(cmd.Example) > 0 {
			cmd.Example = fmt.Sprintf("%s\n\n%s", cmd.Example, v)
		} else {
			cmd.Example = v
		}
	}

	cmd.PersistentFlags().BoolVarP(&c.debug, "debug", "d", false, "debug output")

	cmd.PersistentFlags().BoolVarP(&c.switches.ShowGraphite, "graphite", "", false, "graphite metrics output")
	cmd.PersistentFlags().BoolVarP(&c.switches.Dryrun, "dry-run", "D", false, "dry-run: no monitoring graphite pushes")
	cmd.PersistentFlags().BoolVarP(&c.switches.ShowJuggler, "juggler", "", false, "juggler output")
	cmd.PersistentFlags().BoolVarP(&c.switches.ShowStatus, "status", "", true, "textual statuses metrics output")
	cmd.PersistentFlags().BoolVarP(&c.switches.GenerateJugglerConfig, "juggler-config", "",
		false, "generate juggler config")

	cmd.PersistentFlags().BoolVarP(&c.switches.Crlf, "crlf", "L", false, "crlf output")

	// listing monitor checks (as a call to service
	// process via unix socket):
	// <cmd> monitor list <check-id> [switches]"
	monitorListCmd := cmdMonitorList{}
	monitorListCmd.s = c
	cmd.AddCommand(monitorListCmd.Command())

	// run a monitor check (as a client):
	// <cmd> monitor run <check-id>
	monitorRunCmd := cmdMonitorRun{}
	monitorRunCmd.s = c
	cmd.AddCommand(monitorRunCmd.Command())

	return cmd
}

func (c *cmdMonitor) CheckArgs(cmd *cobra.Command, args []string, minArgs int, maxArgs int) (bool, error) {
	if len(args) < minArgs || (maxArgs != -1 && len(args) > maxArgs) {
		cmd.Help()
		if len(args) == 0 {
			return true, nil
		}
		return true, fmt.Errorf("Invalid number of arguments")
	}
	return false, nil
}

func (c *cmdMonitor) InitCommand(cmd *cobra.Command, args []string) (string, error) {

	var exit bool
	c.InitDebugLog(cmd, args)

	// Checking the possible number of arguments
	if exit, _ = c.CheckArgs(cmd, args, 0, 1); exit {
		return "", errors.New(fmt.Sprintf("Incorrect number of arguments"))
	}

	item := ""
	if len(args) > 0 {
		item = args[0]
	}

	return item, nil
}

func (c *cmdMonitor) InitDebugLog(cmd *cobra.Command, args []string) error {
	var err error

	loglevel := "info"
	if c.debug {
		loglevel = "debug"
	}
	if c.monitor.log.Level, err = logrus.ParseLevel(loglevel); err != nil {
		return err
	}
	return nil
}

// command: "<cmd> events list"
type cmdMonitorList struct {
	// back reference
	s *cmdMonitor
}

func (c *cmdMonitorList) Command() *cobra.Command {
	cmd := &cobra.Command{}

	cmd.Use = "list"
	cmd.Short = "Listing config or monitor checks"
	cmd.Long = "Listing configuration or monitoring results events"

	cmd.RunE = c.Run
	return cmd
}

func (c *cmdMonitorList) Run(cmd *cobra.Command, args []string) error {
	var item string
	var err error
	var code int

	if item, err = c.s.InitCommand(cmd, args); err != nil {
		return err
	}

	c.s.monitor.log.Debug(fmt.Sprintf("%s monitor/list command received, item:'%s'",
		utils.GetGPid(), item))

	if c.s.switches.GenerateJugglerConfig {
		c.s.monitor.JugglerConfig()
		return nil
	}

	var checks []Check
	if checks, code, err = c.s.monitor.apiClientGetChecks(item); err != nil {
		// If some errors occured while receiving data, we
		// need to generate critical/warning stub message in juggler
		c.s.monitor.JugglerStubMessage(2, fmt.Sprintf("monitoring request failed, err:%s", err), item)

		c.s.monitor.log.Debug(fmt.Sprintf("%s monitoring request failed, item:'%s', code:'%d', err:'%s'",
			utils.GetGPid(), item, code, err))

		// returning nil so no any error messages for output
		return nil
	}

	c.s.monitor.JugglerShowChecker(checks[0])

	return nil
}

// command: "<cmd> monitor run"
type cmdMonitorRun struct {
	// back reference
	s    *cmdMonitor
	exec bool
}

func (c *cmdMonitorRun) Command() *cobra.Command {
	cmd := &cobra.Command{}

	cmd.Use = "run"
	cmd.Short = "Running a check"
	cmd.Long = "Running a monitoring check as a client"

	cmd.PersistentFlags().BoolVarP(&c.exec, "exec", "E", false, "executing some monitoring actions (if any)")

	cmd.RunE = c.Run
	return cmd
}

func (c *cmdMonitorRun) Run(cmd *cobra.Command, args []string) error {
	var item string
	var err error

	if item, err = c.s.InitCommand(cmd, args); err != nil {
		return err
	}
	c.s.monitor.log.Debug(fmt.Sprintf("%s monitor/run command received, item:'%s'",
		utils.GetGPid(), item))

	// Executing as a command line call
	if err = c.s.monitor.Exec(item, c.exec); err != nil {
		c.s.monitor.log.Error(fmt.Sprintf("[%d] error running monitoring checks, err:'%s'",
			utils.GetGID(), err))
		return err
	}

	return nil
}
