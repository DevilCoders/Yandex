package events

import (
	"errors"
	"fmt"

	"github.com/sirupsen/logrus"
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cdn/m3/utils"
)

type cmdEvents struct {
	events *Events

	// Class of events to list
	class   string
	c_error bool
	debug   bool
}

func (c *cmdEvents) Command() *cobra.Command {

	// Main subcommand:server
	cmd := &cobra.Command{}
	cmd.Use = "events --- "
	cmd.Short = "Listing monitoring and manging events"
	cmd.Long = `
Events could be pushed from any place of a program
and collected by events module as shared map. Events
are garbage collected later. Normal and force modes
are supported (see below). Listing and monitoring
are provided. Special event "error" type are triggered
as critical monitoring issue, all the others as "info"
`

	var examples = []string{
		`  a) listing error events (with debug)
      $program events listing --debug --error`,
		`  b) forcing garbage collector from command line
      (clearing all events)
      $program events gc --force --debug`,
	}

	for _, v := range examples {
		if len(cmd.Example) > 0 {
			cmd.Example = fmt.Sprintf("%s\n\n%s", cmd.Example, v)
		} else {
			cmd.Example = v
		}
	}

	cmd.PersistentFlags().BoolVarP(&c.debug, "debug", "d", false, "debug output")
	cmd.PersistentFlags().StringVarP(&c.class, "class", "c", "", "class of events")
	cmd.PersistentFlags().BoolVarP(&c.c_error, "error", "E", false, "error class of events")

	// listing events: "events list"
	eventsListCmd := cmdEventsList{}
	eventsListCmd.s = c
	cmd.AddCommand(eventsListCmd.Command())

	// gc (garbage collector) events: "events gc"
	eventsGCCmd := cmdEventsGC{}
	eventsGCCmd.s = c
	cmd.AddCommand(eventsGCCmd.Command())

	// monitoring events: "events monitor"
	eventsMonitorCmd := cmdEventsMonitor{}
	eventsMonitorCmd.s = c
	cmd.AddCommand(eventsMonitorCmd.Command())

	return cmd
}

func (c *cmdEvents) CheckArgs(cmd *cobra.Command, args []string, minArgs int, maxArgs int) (bool, error) {
	if len(args) < minArgs || (maxArgs != -1 && len(args) > maxArgs) {
		cmd.Help()
		if len(args) == 0 {
			return true, nil
		}
		return true, fmt.Errorf("Invalid number of arguments")
	}
	return false, nil
}

func (c *cmdEvents) InitCommand(cmd *cobra.Command, args []string) (string, error) {

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

func (c *cmdEvents) InitDebugLog(cmd *cobra.Command, args []string) error {
	var err error

	loglevel := "info"
	if c.debug {
		loglevel = "debug"
	}
	if c.events.log.Level, err = logrus.ParseLevel(loglevel); err != nil {
		return err
	}
	return nil
}

// command: "<cmd> events list"
type cmdEventsList struct {
	// back reference
	s *cmdEvents

	// Class of events to list
	class string
}

func (c *cmdEventsList) Command() *cobra.Command {
	cmd := &cobra.Command{}

	cmd.Use = "list"
	cmd.Short = "Listing events"
	cmd.Long = "Listing events"

	cmd.RunE = c.Run
	return cmd
}

func (c *cmdEventsList) Run(cmd *cobra.Command, args []string) error {
	var item string
	var err error

	if item, err = c.s.InitCommand(cmd, args); err != nil {
		return err
	}

	c.s.events.log.Debug(fmt.Sprintf("[%d] events/list list starting, item:'%s', class:'%s'",
		utils.GetGID(), item, c.s.class))

	if c.s.c_error {
		c.s.class = "error"
	}
	if _, _, err = c.s.events.apiClientGetEvents(c.s.class); err != nil {
		c.s.events.log.Error(fmt.Sprintf("[%d] error getting events, err:'%s'",
			utils.GetGID(), err))
		return err
	}

	return nil
}

// command: "<cmd> events gc"
type cmdEventsGC struct {
	// back reference
	s *cmdEvents

	// Class of events to list
	class string

	force bool
}

func (c *cmdEventsGC) Command() *cobra.Command {
	cmd := &cobra.Command{}

	cmd.Use = "gc"
	cmd.Short = "Garbage collector"
	cmd.Long = "Garbaging events collector"

	cmd.PersistentFlags().BoolVarP(&c.force, "force", "F", false, "force output")

	cmd.RunE = c.Run
	return cmd
}

func (c *cmdEventsGC) Run(cmd *cobra.Command, args []string) error {
	var err error

	c.s.events.log.Debug(fmt.Sprintf("[%d] events/gc garbage collector starting", utils.GetGID()))

	if err = c.s.events.apiClientGC(c.force); err != nil {
		c.s.events.log.Error(fmt.Sprintf("[%d] error garbage collector, err:'%s'",
			utils.GetGID(), err))
		return err
	}

	return nil
}

// command: "<cmd> events monitor"
type cmdEventsMonitor struct {
	// back reference
	s *cmdEvents

	// item to monitor (class of monitoring?),
	// could be "events" by default
	item string

	// monitor for juggler/graphite events
	// processing via command line or cron
	monitor Monitor
}

func (c *cmdEventsMonitor) Command() *cobra.Command {
	cmd := &cobra.Command{}

	cmd.Use = "monitor"
	cmd.Short = "Monitoring events queue"
	cmd.Long = `
Monitoring and managing events queues: (a) displaying
monrun/juggler monitored events (b) generating a juggler
configuration for monitored items (c) sending raw-events
into juggler local-based queue (d) sending graphite
metrics
`

	var examples = []string{
		`  a) listing montitored events (with debug)
      %s events monitor --debug events`,
		`  b) displaying monrun/juggler configuration for
      "events" item
      %s events monitor --juggler-config events`,
	}

	for _, v := range examples {
		vv := fmt.Sprintf(v, c.s.events.program)
		if len(cmd.Example) > 0 {
			cmd.Example = fmt.Sprintf("%s\n\n%s", cmd.Example, vv)
		} else {
			cmd.Example = vv
		}
	}

	cmd.PersistentFlags().BoolVarP(&c.monitor.sw.ShowGraphite, "graphite", "", false, "graphite metrics output")
	cmd.PersistentFlags().BoolVarP(&c.monitor.sw.ShowJuggler, "juggler", "", false, "juggler output")
	cmd.PersistentFlags().BoolVarP(&c.monitor.sw.ShowStatus, "status", "", true, "textual statuses metrics output")
	cmd.PersistentFlags().BoolVarP(&c.monitor.sw.GenerateJugglerConfig, "juggler-config", "", false, "generate juggler config")

	cmd.RunE = c.Run
	return cmd
}

func (c *cmdEventsMonitor) Run(cmd *cobra.Command, args []string) error {
	var item string
	var err error

	// One more back reference to events
	c.monitor.events = c.s.events

	if item, err = c.s.InitCommand(cmd, args); err != nil {
		return err
	}

	if len(item) == 0 {
		item = "events"
	}

	c.s.events.log.Debug(fmt.Sprintf("[%d] events monitoring, class:'%s', item:'%s'",
		utils.GetGID(), c.s.events.class, item))

	if c.monitor.sw.GenerateJugglerConfig {
		c.monitor.ShowItem(c.s.events.class, item, "", JUGGLER_OK)
		return nil
	}

	switch item {
	case "events":
		c.monitor.Events()
	}

	return nil
}
