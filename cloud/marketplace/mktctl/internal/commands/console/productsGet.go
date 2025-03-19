package console

import (
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/logger"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/marketplace"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/pretty"

	"github.com/spf13/cobra"
	"go.uber.org/zap"
)

func init() {
	productsGet.Flags().StringVar(&productID, "product-id", "", "")
	_ = productsGet.MarkFlagRequired("product-id")

	consoleCmd.AddCommand(productsGet)
}

var productsGet = &cobra.Command{
	Use:  "products-get",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		cfg := config.FromContext(ctx)
		mkt := marketplace.NewClient(cfg.GetToken(), cfg.GetMarketplaceConsoleEndpoint())

		resp, err := mkt.NewRequest().
			SetPathParam("product_id", productID).
			Get("/products/{product_id}")
		if err != nil {
			logger.FatalCtx(ctx, "products-get", zap.Error(err))
		}

		if err = pretty.Print(resp.Body(), cfg.Format); err != nil {
			logger.FatalCtx(ctx, "products-get", zap.Error(err))
		}
	},
}
