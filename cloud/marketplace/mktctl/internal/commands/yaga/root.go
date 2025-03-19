package yaga

import "github.com/spf13/cobra"

var (
	yagaCmd = cobra.Command{Use: "yaga"}

	instanceID  = ""
	requestID   = ""
	keypath     = ""
	KeyFilename = "yaga-pwd-reset.key"
	username    = ""
)

func AddCommands(cmd *cobra.Command) {
	cmd.AddCommand(&yagaCmd)
}
