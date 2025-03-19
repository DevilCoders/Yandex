package publishers

import "github.com/spf13/cobra"

var publishersCmd = cobra.Command{Use: "publishers"}

func AddCommands(cmd *cobra.Command) {
	cmd.AddCommand(&publishersCmd)
}
