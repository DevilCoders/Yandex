package proposals

import (
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/logger"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/marketplace"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/pretty"

	"github.com/spf13/cobra"
	"go.uber.org/zap"
)

func init() {
	delete.Flags().StringVar(&proposalID, "proposal-id", "", "")
	_ = delete.MarkFlagRequired("proposal-id")

	proposalsCmd.AddCommand(delete)
}

var delete = &cobra.Command{
	Use:  "delete",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		cfg := config.FromContext(ctx)
		mkt := marketplace.NewClient(cfg.GetToken(), cfg.GetMarketplacePartnersEndpoint())

		resp, err := mkt.NewRequest().
			SetPathParam("publisher_id", publisherID).
			SetPathParam("product_id", productID).
			SetPathParam("proposal_id", proposalID).
			Delete("/publishers/{publisher_id}/products/{product_id}/proposals/{proposal_id}")
		if err != nil {
			logger.FatalCtx(ctx, "delete", zap.Error(err))
		}

		if err = pretty.Print(resp.Body(), cfg.Format); err != nil {
			logger.FatalCtx(ctx, "delete", zap.Error(err))
		}
	},
}
