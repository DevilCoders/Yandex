package blueprints

import "github.com/spf13/cobra"

var (
	blueprintsCmd = cobra.Command{Use: "blueprints"}
	blueprintID   string
)

func AddCommands(cmd *cobra.Command) {
	cmd.AddCommand(&blueprintsCmd)
}
