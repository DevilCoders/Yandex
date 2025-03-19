package proposals

import "github.com/spf13/cobra"

var (
	proposalsCmd = cobra.Command{Use: "proposals"}
	publisherID  string
	productID    string
	proposalID   string
	newVersionID string
)

func AddCommands(cmd *cobra.Command) {
	proposalsCmd.PersistentFlags().StringVar(&publisherID, "publisher-id", "", "")
	_ = proposalsCmd.MarkPersistentFlagRequired("publisher-id")

	proposalsCmd.PersistentFlags().StringVar(&productID, "product-id", "", "")
	_ = proposalsCmd.MarkPersistentFlagRequired("product-id")

	cmd.AddCommand(&proposalsCmd)
}
