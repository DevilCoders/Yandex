package console

import "github.com/spf13/cobra"

var (
	consoleCmd = cobra.Command{Use: "console"}
	productID  string
)

func AddCommands(cmd *cobra.Command) {
	cmd.AddCommand(&consoleCmd)
}
