package cmd

import (
	"fmt"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cdn/m3/cmd/config"
	"a.yandex-team.ru/cdn/m3/utils"
)

type cmdConfig struct {
	g *config.CmdGlobal
}

func (c *cmdConfig) Command() *cobra.Command {

	// Main subcommand:server
	cmd := &cobra.Command{}
	cmd.Use = "config"
	cmd.Short = "Showing (and managing runtime) configuration"
	cmd.Long = `Description:
  Showing configuration
`
	// getting current configuration (runtime) "d2 config show"
	showCmdConfig := cmdConfigShow{g: c.g}
	showCmdConfig.cc = c
	cmd.AddCommand(showCmdConfig.Command())

	return cmd
}

// command: "d2 config show"
type cmdConfigShow struct {
	g *config.CmdGlobal

	cc *cmdConfig
}

func (c *cmdConfigShow) Command() *cobra.Command {
	cmd := &cobra.Command{}

	cmd.Use = "show"
	cmd.Short = "Showing current configuration"
	cmd.Long = "Showing current configuration"

	cmd.RunE = c.Run

	return cmd
}

func (c *cmdConfigShow) Run(cmd *cobra.Command, args []string) error {
	var content string
	var err error

	c.g.Opts.Runtime.Version = version
	c.g.Opts.Runtime.Date = date

	c.g.Log.Debug(fmt.Sprintf("%s debug switch is 'ON'", utils.GetGPid()))

	if c.g.Opts == nil {
		c.g.Log.Error(fmt.Sprintf("%s error detecting current configutation", utils.GetGPid()))
		return err
	}

	if content, err = utils.ConvMarshal(c.g.Opts, "YAML"); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error marshalling configuration into yaml, err:'%s'",
			utils.GetGPid(), err))
		return err
	}

	fmt.Printf("%s\n", content)
	return nil
}
