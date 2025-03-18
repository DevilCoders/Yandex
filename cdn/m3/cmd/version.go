package cmd

import (
	"fmt"
	"runtime"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cdn/m3/cmd/config"
	"a.yandex-team.ru/cdn/m3/utils"
)

var version string
var date string

type cmdVersion struct {
	g *config.CmdGlobal
}

func SetVersion(ver string) {
	version = ver
}

func SetDate(da string) {
	date = da
}

func (c *cmdVersion) Command() *cobra.Command {

	// Main subcommand:server
	cmd := &cobra.Command{}
	cmd.Use = "version"
	cmd.Short = "Showing version and some possible useful information"
	cmd.Long = `Description:
  Showing version and runtime options
`
	cmd.RunE = c.Run

	return cmd
}

func (c *cmdVersion) Run(cmd *cobra.Command, args []string) error {
	c.g.Log.Info(fmt.Sprintf("%s %s: '%s', build date: '%s', compiler: '%s' '%s', running at '%s'",
		utils.GetGPid(), c.g.Opts.Runtime.ProgramName, version,
		date, runtime.Compiler, runtime.Version(),
		c.g.Opts.Runtime.Hostname))
	return nil
}
