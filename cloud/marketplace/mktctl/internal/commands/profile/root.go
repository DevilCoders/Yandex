package profile

import "github.com/spf13/cobra"

var profileCmd = cobra.Command{Use: "profile"}

func AddCommands(cmd *cobra.Command) {
	cmd.AddCommand(&profileCmd)
}
