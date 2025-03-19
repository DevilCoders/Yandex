package operations

import "github.com/spf13/cobra"

var (
	operationsCmd = cobra.Command{Use: "operations"}
	operationID   string
)

func AddCommands(cmd *cobra.Command) {
	cmd.AddCommand(&operationsCmd)
}
