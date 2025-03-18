package cmd

// mgmt commands (1) mgmt bgp (2)  mgmt data

import ( //	"errors"
	"fmt"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cdn/m3/cmd/config"
	"a.yandex-team.ru/cdn/m3/core"
	"a.yandex-team.ru/cdn/m3/utils"
)

type cmdMgmt struct {
	g *config.CmdGlobal

	// dry-run: no any actions are done
	swDryrun bool

	// back reference for core created (filled in root command)
	core *core.Core
}

func (c *cmdMgmt) Command() *cobra.Command {

	cmd := &cobra.Command{}
	cmd.Use = "mgmt"
	cmd.Short = "m3 managment operations for server and command line"
	cmd.Long = `Description:
  m3 mgmt implements a list of actions (1) mgmt bgp processing (specifically
  for debugging purposes) (2) mgmt sync network objects (links), (routes),
  (rules) and bgp configuration (3) mgmt flush network objects

Possible switches and their values:
      --dry-run    no actions are done actually
`
	cmd.PersistentFlags().BoolVarP(&c.swDryrun, "dry-run", "D",
		false, "dry-run: no any actual actions are done")

	var examples = []string{
		`  a) starting bgp session for peers configuration, waiting
     for bgp session establish status and generating current
     network objects routes, rules and links

     m3 mgmt bgp start --debug --timeout=30`,
		`  b) syncing data from bgp session: waiting for bgp session,
     generating networks objects, syncing bgp sessions with networking ip
     route objects

     m3 mgmt sync --debug  --timeout=20 [--dry-run]`,
		`  c) flushing all network objects on target system, with
     dry-run flushing all operations are prepared but no actions are done

     m3 mgmt flush --debug  [--dry-run]`,
	}

	for _, v := range examples {
		if len(cmd.Example) > 0 {
			cmd.Example = fmt.Sprintf("%s\n\n%s", cmd.Example, v)
		} else {
			cmd.Example = v
		}
	}

	// mgmt bgp [commands]
	mgmtBgpCmd := cmdMgmtBgp{g: c.g}
	mgmtBgpCmd.s = c
	cmd.AddCommand(mgmtBgpCmd.Command())

	// mgmt sync [commands]
	mgmtSyncCmd := cmdMgmtSync{g: c.g}
	mgmtSyncCmd.s = c
	cmd.AddCommand(mgmtSyncCmd.Command())

	// mgmt flush [commands]
	mgmtFlushCmd := cmdMgmtFlush{g: c.g}
	mgmtFlushCmd.s = c
	cmd.AddCommand(mgmtFlushCmd.Command())

	return cmd
}

// mgmt bgp [commands]
type cmdMgmtBgp struct {
	g *config.CmdGlobal

	// back reference
	s *cmdMgmt
}

func (c *cmdMgmtBgp) Command() *cobra.Command {
	cmd := &cobra.Command{}

	cmd.Use = "bgp"
	cmd.Short = "Managing and debugging bgp sessions"
	cmd.Long = "Managing and debugging bgp sessions"

	// mgmt bgp start
	objectsBgpStartCmd := cmdMgmtBgpStart{g: c.g}
	objectsBgpStartCmd.s = c.s
	cmd.AddCommand(objectsBgpStartCmd.Command())

	return cmd
}

// command: "cc-core objects pipeline show"
type cmdMgmtBgpStart struct {
	g *config.CmdGlobal

	// back reference
	s *cmdMgmt

	// timeout
	Timeout int
}

func (c *cmdMgmtBgpStart) Command() *cobra.Command {
	cmd := &cobra.Command{}

	cmd.Use = "start"
	cmd.Short = "Starting bgp session and watching bgp messages"
	cmd.Long = "Starting bgp session and watching bgp messages"

	cmd.PersistentFlags().IntVarP(&c.Timeout, "timeout", "T", 20,
		"command line bgp session timeout seconds, default: 20")

	cmd.RunE = c.Run
	return cmd
}

func (c *cmdMgmtBgpStart) Run(cmd *cobra.Command, args []string) error {
	var err error
	id := "(cmd)"
	c.g.Log.Debug(fmt.Sprintf("%s %s mgmt bgp start", utils.GetGPid(), id))

	var task *core.Task
	if task, err = core.CreateTask(c.g, core.TASK_NULL); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error creating bgp start task, err:'%s'",
			utils.GetGPid(), err))
		return err
	}
	task.Core = c.s.core

	var BgpOptionsOverrides core.BgpOptionsOverrides
	BgpOptionsOverrides.Timeout = c.Timeout

	if err = task.ExecBgpStart(&BgpOptionsOverrides); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error executing bgp start task, err:'%s'",
			utils.GetGPid(), err))
		return err
	}

	return err
}

// mgmt sync [commands]
type cmdMgmtSync struct {
	g *config.CmdGlobal

	// back reference
	s *cmdMgmt

	// timeout
	Timeout int
}

func (c *cmdMgmtSync) Command() *cobra.Command {
	cmd := &cobra.Command{}

	cmd.Use = "sync"
	cmd.Short = "Syncing bgp session and network objects"
	cmd.Long = "Syncing bgp session and network objects"

	cmd.PersistentFlags().IntVarP(&c.Timeout, "timeout", "T", 20,
		"command line bgp session timeout seconds, default: 20")

	cmd.RunE = c.Run
	return cmd
}

func (c *cmdMgmtSync) Run(cmd *cobra.Command, args []string) error {
	var err error
	id := "(sync)"
	c.g.Log.Debug(fmt.Sprintf("%s %s mgmt sync", utils.GetGPid(), id))

	var task *core.Task
	if task, err = core.CreateTask(c.g, core.TASK_NULL); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error creating bgp start task, err:'%s'",
			utils.GetGPid(), err))
		return err
	}
	task.Core = c.s.core

	var SyncOptionsOverrides core.SyncOptionsOverrides
	SyncOptionsOverrides.BgpStart = true
	SyncOptionsOverrides.Dryrun = c.s.swDryrun
	SyncOptionsOverrides.Timeout = c.Timeout

	if err = task.ExecSync(&SyncOptionsOverrides); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error executing bgp start task, err:'%s'",
			utils.GetGPid(), err))
		return err
	}

	return err
}

// mgmt flush [commands]
type cmdMgmtFlush struct {
	g *config.CmdGlobal

	// back reference
	s *cmdMgmt
}

func (c *cmdMgmtFlush) Command() *cobra.Command {
	cmd := &cobra.Command{}

	cmd.Use = "flush"
	cmd.Short = "Flushing network configuration on host"
	cmd.Long = "Flushing network configuration on host"

	cmd.RunE = c.Run
	return cmd
}

func (c *cmdMgmtFlush) Run(cmd *cobra.Command, args []string) error {
	var err error
	id := "(flush)"
	c.g.Log.Debug(fmt.Sprintf("%s %s mgmt flush", utils.GetGPid(), id))

	var task *core.Task
	if task, err = core.CreateTask(c.g, core.TASK_NULL); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error creating bgp start task, err:'%s'",
			utils.GetGPid(), err))
		return err
	}
	task.Core = c.s.core

	var SyncOptionsOverrides core.SyncOptionsOverrides
	SyncOptionsOverrides.Dryrun = c.s.swDryrun

	if err = task.ExecFlush(&SyncOptionsOverrides); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error executing flush network configuration, err:'%s'",
			utils.GetGPid(), err))
		return err
	}

	return err
}
