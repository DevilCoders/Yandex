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
	reject.Flags().StringVar(&proposalID, "proposal-id", "", "")
	_ = reject.MarkFlagRequired("proposal-id")

	proposalsCmd.AddCommand(reject)
}

var reject = &cobra.Command{
	Use:  "reject",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		cfg := config.FromContext(ctx)
		mkt := marketplace.NewClient(cfg.GetToken(), cfg.GetMarketplacePrivateEndpoint())

		resp, err := mkt.NewRequest().
			SetPathParam("publisher_id", publisherID).
			SetPathParam("product_id", productID).
			SetPathParam("proposal_id", productID).
			Get("/publishers/{publisher_id}/products/{product_id}/proposals/{proposal_id}:reject")
		if err != nil {
			logger.FatalCtx(ctx, "reject", zap.Error(err))
		}

		if err = pretty.Print(resp.Body(), cfg.Format); err != nil {
			logger.FatalCtx(ctx, "reject", zap.Error(err))
		}
	},
}
