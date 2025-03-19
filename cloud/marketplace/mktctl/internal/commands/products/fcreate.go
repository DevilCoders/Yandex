package products

import (
	"fmt"
	"os"

	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/logger"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/marketplace"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/pretty"

	"github.com/spf13/cobra"
	"go.uber.org/zap"
)

func init() {
	productsCmd.AddCommand(fcreate)
}

var fcreate = &cobra.Command{
	Use:  "fcreate",
	Args: cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()

		req, err := os.ReadFile(args[0])
		if err != nil {
			logger.FatalCtx(ctx, "read file", zap.Error(err))
		}

		cfg := config.FromContext(ctx)
		mkt := marketplace.NewClient(cfg.GetToken(), cfg.GetMarketplacePartnersEndpoint())

		resp, err := mkt.NewRequest().
			SetBody(req).
			SetPathParam("publisher_id", publisherID).
			Post("/publishers/{publisher_id}/products")
		if err != nil {
			logger.FatalCtx(ctx, "create", zap.Error(err))
		}

		if err = pretty.Print(resp.Body(), cfg.Format); err != nil {
			fmt.Println(string(resp.Body()))
			logger.FatalCtx(ctx, "print", zap.Error(err))
		}
	},
}
