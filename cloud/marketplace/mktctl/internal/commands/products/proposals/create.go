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
	create.Flags().StringVar(&newVersionID, "new-version-id", "", "")
	_ = create.MarkFlagRequired("new-version-id")

	proposalsCmd.AddCommand(create)
}

var create = &cobra.Command{
	Use:  "create",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		cfg := config.FromContext(ctx)
		mkt := marketplace.NewClient(cfg.GetToken(), cfg.GetMarketplacePartnersEndpoint())

		//nolint:ST1003
		resp, err := mkt.NewRequest().
			SetBody(struct {
				NewVersionID string `json:"newVersionId"`
			}{
				NewVersionID: newVersionID,
			}).
			SetPathParam("publisher_id", publisherID).
			SetPathParam("product_id", productID).
			Post("/publishers/{publisher_id}/products/{product_id}/proposals")
		if err != nil {
			logger.FatalCtx(ctx, "create", zap.Error(err))
		}

		if err = pretty.Print(resp.Body(), cfg.Format); err != nil {
			logger.FatalCtx(ctx, "create", zap.Error(err))
		}
	},
}
