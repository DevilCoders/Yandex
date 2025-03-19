package products

import (
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/commands/products/proposals"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/commands/products/versions"

	"github.com/spf13/cobra"
)

var (
	productsCmd    = cobra.Command{Use: "products"}
	publisherID    string
	productID      string
	autoApprove    bool
	autoApply      bool
	validBaseImage bool
)

func AddCommands(cmd *cobra.Command) {

	productsCmd.PersistentFlags().StringVar(&publisherID, "publisher-id", "", "")
	_ = productsCmd.MarkPersistentFlagRequired("publisher-id")

	proposals.AddCommands(&productsCmd)
	versions.AddCommands(&productsCmd)

	cmd.AddCommand(&productsCmd)
}
