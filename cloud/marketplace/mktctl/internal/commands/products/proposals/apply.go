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
	apply.Flags().StringVar(&proposalID, "proposal-id", "", "")
	_ = apply.MarkFlagRequired("proposal-id")

	proposalsCmd.AddCommand(apply)
}

var apply = &cobra.Command{
	Use:  "apply",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		cfg := config.FromContext(ctx)
		mkt := marketplace.NewClient(cfg.GetToken(), cfg.GetMarketplacePartnersEndpoint())

		resp, err := mkt.NewRequest().
			SetPathParam("publisher_id", publisherID).
			SetPathParam("product_id", productID).
			SetPathParam("proposal_id", proposalID).
			Post("/publishers/{publisher_id}/products/{product_id}/proposals/{proposal_id}:apply")
		if err != nil {
			logger.FatalCtx(ctx, "apply", zap.Error(err))
		}

		if err = pretty.Print(resp.Body(), cfg.Format); err != nil {
			logger.FatalCtx(ctx, "apply", zap.Error(err))
		}
	},
}
