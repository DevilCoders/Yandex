package cmd

// status command

import (
	"errors"
	"fmt"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cdn/m3/cmd/config"
	"a.yandex-team.ru/cdn/m3/core"
	"a.yandex-team.ru/cdn/m3/utils"
)

type cmdStatus struct {
	g *config.CmdGlobal

	// back reference for core created (filled in root command)
	core *core.Core

	// json output
	json bool
}

func (c *cmdStatus) Command() *cobra.Command {

	cmd := &cobra.Command{}
	cmd.Use = "status"
	cmd.Short = "Status command returns state information"
	cmd.Long = `Description:
  m3 status for extended information (1) bgp session objects (a) links
  (b) prefixes (c) neigbors; (2) networks objects: links, routes and rules
  `
	cmd.PersistentFlags().BoolVarP(&c.json, "json", "J", false, "json output")

	var examples = []string{
		`  a) getting status with full debug information about current
     network and bgp operations states

     m3 status --json --debug`,
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

func (c *cmdStatus) Run(cmd *cobra.Command, args []string) error {
	var err error
	id := "(cmd)"
	c.g.Log.Debug(fmt.Sprintf("%s %s status command", utils.GetGPid(), id))

	var task *core.Task
	if task, err = core.CreateTask(c.g, core.TASK_NULL); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error creating bgp start task, err:'%s'",
			utils.GetGPid(), err))
		return err
	}

	if c.core == nil {
		err = errors.New("core has not beed initailized")
		c.g.Log.Error(fmt.Sprintf("%s could run command, err:'%s'",
			utils.GetGPid(), err))
		return err
	}

	task.Core = c.core

	var StatusOptionsOverrides core.StatusOptionsOverrides
	StatusOptionsOverrides.Json = c.json
	StatusOptionsOverrides.Info = true

	if err = task.ExecStatus(&StatusOptionsOverrides); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error executing status command, err:'%s'",
			utils.GetGPid(), err))
		return err
	}

	return err
}
