package builds

import "github.com/spf13/cobra"

var (
	buildsCmd = cobra.Command{Use: "builds"}
	buildID   string
)

func AddCommands(cmd *cobra.Command) {
	cmd.AddCommand(&buildsCmd)
}
