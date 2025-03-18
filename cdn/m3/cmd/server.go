package cmd

import (
	"errors"
	"fmt"
	"os"

	"github.com/erikdubbelboer/gspt"
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cdn/m3/cmd/config"
	"a.yandex-team.ru/cdn/m3/core"
	"a.yandex-team.ru/cdn/m3/utils"
)

type cmdServer struct {
	g *config.CmdGlobal

	debug bool

	// back reference for core created (filled in root command)
	core *core.Core
}

func (c *cmdServer) Command() *cobra.Command {

	// Main subcommand:server
	cmd := &cobra.Command{}
	cmd.Use = "server --- "
	cmd.Short = "Controlling server process"
	cmd.Long = `Description:
  m3 service is started as a process under systemd control or as
  stand-alone server process (e.g. for rtc containers).
`
	cmd.PersistentFlags().BoolVarP(&c.debug, "debug", "d", false, "debug output")

	var examples = []string{
		`  a) rtc: starting m3 service as a standalone process, we define
     such mode as detached, in debug mode, please note that
     log is defined in /etc/m3/m3.yaml

     m3 server start --detach --debug`,

		`  b) rtc: stopping m3 as a detached process, we operate here with
     pid file, stored at start on /var/run/m3.pid

     m3 server stop --detach --debug`,
	}

	for _, v := range examples {
		if len(cmd.Example) > 0 {
			cmd.Example = fmt.Sprintf("%s\n\n%s", cmd.Example, v)
		} else {
			cmd.Example = v
		}
	}

	// starting m3 server: "m3 server start"
	serverStartCmd := cmdServerStart{g: c.g}
	serverStartCmd.s = c
	cmd.AddCommand(serverStartCmd.Command())

	// stopping m3 server: "m3 server stop"
	serverStopCmd := cmdServerStop{g: c.g}
	serverStopCmd.s = c
	cmd.AddCommand(serverStopCmd.Command())

	// executing task via m3 server exec: "m3 server exec"
	serverExecCmd := cmdServerExec{g: c.g}
	serverExecCmd.s = c
	cmd.AddCommand(serverExecCmd.Command())

	// logrotate signal
	serverLogrotateCmd := cmdServerLogrotate{g: c.g}
	serverLogrotateCmd.s = c
	cmd.AddCommand(serverLogrotateCmd.Command())

	return cmd
}

// Init command that is called from any commands
func (c *cmdServer) InitCommand(cmd *cobra.Command, args []string) (string, error) {

	//c.InitDebugLog(cmd, args)

	// Checking the possible number of arguments
	exit, _ := c.g.CheckArgs(cmd, args, 1, 1)
	if exit {
		return "", errors.New(fmt.Sprintf("Incorrect number of arguments"))
	}

	// Getting item name (if supplied)
	item := ""
	if len(args) > 0 {
		item = args[0]
	}

	return item, nil
}

// command: "m3 server start"
type cmdServerStart struct {
	g *config.CmdGlobal
	s *cmdServer

	detach bool
}

func (c *cmdServerStart) Command() *cobra.Command {
	cmd := &cobra.Command{}

	cmd.Use = "start"
	cmd.Short = "Starting server"
	cmd.Long = "Starting server"

	// detach should be used in non-systemd run
	cmd.PersistentFlags().BoolVarP(&c.detach, "detach", "D", false, "detach process")

	cmd.RunE = c.Run
	return cmd
}

func (c *cmdServerStart) Run(cmd *cobra.Command, args []string) error {

	program := c.g.Opts.Runtime.ProgramName
	gspt.SetProcTitle(fmt.Sprintf("%s: waiting for jobs",
		program))

	C, err := core.CreateCore(c.g)
	if err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error creating core, err:'%s'",
			utils.GetGPid(), err))
		return err
	}

	var Overrides core.CoreOverrides
	Overrides.Detach = c.detach
	if err := C.StartCore(&Overrides); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error starting core, err:'%s'",
			utils.GetGPid(), err))
		return err
	}

	c.g.Log.Debug(fmt.Sprintf("%s %s service stopped", utils.GetGPid(), program))
	return nil
}

// command: "m3 server stop"
type cmdServerStop struct {
	g *config.CmdGlobal
	s *cmdServer

	detach bool
}

func (c *cmdServerStop) Command() *cobra.Command {
	cmd := &cobra.Command{}

	cmd.Use = "stop"
	cmd.Short = "Stopping server"
	cmd.Long = "Stopping server"

	// detach should be used in non-systemd run
	cmd.PersistentFlags().BoolVarP(&c.detach, "detach", "D", false, "detach process")

	cmd.RunE = c.Run
	return cmd
}

func (c *cmdServerStop) Run(cmd *cobra.Command, args []string) error {

	program := c.g.Opts.Runtime.ProgramName
	c.g.Log.Debug(fmt.Sprintf("%s '%s' request to stop",
		utils.GetGPid(), program))

	C, err := core.CreateCore(c.g)
	if err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error creating core, err:'%s'",
			utils.GetGPid(), err))
		return err
	}

	var Overrides core.CoreOverrides
	Overrides.Detach = c.detach
	if err := C.StopCore(&Overrides); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error stopping core, err:'%s'",
			utils.GetGPid(), err))
		return err
	}

	c.g.Log.Debug(fmt.Sprintf("%s %s service stopped", utils.GetGPid(), program))
	return nil
}

// command: "m3 server exec <json:task>"
type cmdServerExec struct {
	g *config.CmdGlobal
	s *cmdServer
}

func (c *cmdServerExec) Command() *cobra.Command {
	cmd := &cobra.Command{}

	cmd.Use = "exec"
	cmd.Short = "Executing a task on server side"
	cmd.Long = "Executing a task on server side, forking new process to complete json:task command"

	cmd.RunE = c.Run
	return cmd
}

func (c *cmdServerExec) Run(cmd *cobra.Command, args []string) error {
	var err error
	var item string

	// Initialization part: setting log and parameters
	// handling, expecting json task
	item, err = c.s.InitCommand(cmd, args)
	if err != nil {
		return err
	}
	c.g.Log.Debug(fmt.Sprintf("[%d]:[%d] got request to execute task:'%s'",
		os.Getpid(), utils.GetGID(), item))

	var task *core.Task
	if task, err = core.CreateTask(c.g, item); err != nil {
		c.g.Log.Error(fmt.Sprintf("[%d] error creating task, err:'%s'",
			utils.GetGID(), err))
		return err
	}

	if err = task.Exec(); err != nil {
		c.g.Log.Error(fmt.Sprintf("[%d] error executing task, err:'%s'",
			utils.GetGID(), err))
		return err
	}
	return nil
}

// command: "m3 server logrotate"
type cmdServerLogrotate struct {
	g *config.CmdGlobal
	s *cmdServer

	move bool
}

func (c *cmdServerLogrotate) Command() *cobra.Command {
	cmd := &cobra.Command{}

	cmd.Use = "logrotate"
	cmd.Short = "Rotating current log"
	cmd.Long = "Rotating current log, in case of logging in file"

	cmd.PersistentFlags().BoolVarP(&c.move, "move", "M", false,
		"move current log to old and create new log file")

	var examples = []string{
		`  a) rtc: rotating files in detach mode, move
     current log file to old, create new file and set log out
     put to newly created file

     m3 server logrotate --move --debug`,

		`  b) rtc: create new file in place current log
     as it could be rotated by some external rotation logics:
     logrotate, instancectl?

     m3 server logrotate --debug`,
	}

	for _, v := range examples {
		if len(cmd.Example) > 0 {
			cmd.Example = fmt.Sprintf("%s\n\n%s", cmd.Example, v)
		} else {
			cmd.Example = v
		}
	}

	cmd.RunE = c.Run
	return cmd
}

func (c *cmdServerLogrotate) Run(cmd *cobra.Command, args []string) error {
	var err error
	var task *core.Task
	if task, err = core.CreateTask(c.g, ""); err != nil {
		c.g.Log.Error(fmt.Sprintf("[%d] error creating task, err:'%s'",
			utils.GetGID(), err))
		return err
	}
	task.Core = c.s.core

	var LogrotateOptions config.LogrotateOptions
	LogrotateOptions.Move = c.move

	if err = task.Logrotate(&LogrotateOptions); err != nil {
		c.g.Log.Error(fmt.Sprintf("[%d] error executing task, err:'%s'",
			utils.GetGID(), err))
		return err
	}
	return nil
}
