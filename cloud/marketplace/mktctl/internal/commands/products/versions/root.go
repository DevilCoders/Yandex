package versions

import "github.com/spf13/cobra"

var (
	versionsCmd = cobra.Command{Use: "versions"}
	publisherID string
	productID   string
	versionID   string
)

func AddCommands(cmd *cobra.Command) {
	versionsCmd.PersistentFlags().StringVar(&publisherID, "publisher-id", "", "")
	_ = versionsCmd.MarkPersistentFlagRequired("publisher-id")

	versionsCmd.PersistentFlags().StringVar(&productID, "product-id", "", "")
	_ = versionsCmd.MarkPersistentFlagRequired("product-id")

	cmd.AddCommand(&versionsCmd)
}
