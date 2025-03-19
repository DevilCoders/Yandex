package versions

import (
	"os"

	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/logger"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/marketplace"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/pretty"

	"github.com/spf13/cobra"
	"go.uber.org/zap"
)

func init() {
	fupdate.Flags().StringVar(&versionID, "version-id", "", "")
	_ = fupdate.MarkFlagRequired("version-id")

	versionsCmd.AddCommand(fupdate)
}

var fupdate = &cobra.Command{
	Use:  "fupdate",
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
			SetPathParam("product_id", productID).
			SetPathParam("version_id", productID).
			Patch("/publishers/{publisher_id}/products/{product_id}/versions/{version_id}")
		if err != nil {
			logger.FatalCtx(ctx, "fupdate", zap.Error(err))
		}

		if err = pretty.Print(resp.Body(), cfg.Format); err != nil {
			logger.FatalCtx(ctx, "fupdate", zap.Error(err))
		}
	},
}
