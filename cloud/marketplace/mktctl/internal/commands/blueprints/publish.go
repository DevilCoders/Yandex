package blueprints

import (
	"github.com/spf13/cobra"
	"go.uber.org/zap"

	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/logger"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/marketplace"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/pretty"
)

func init() {
	publish.Flags().StringVar(&blueprintID, "blueprint-id", "", "")
	_ = publish.MarkFlagRequired("blueprint-id")

	blueprintsCmd.AddCommand(publish)
}

var publish = &cobra.Command{
	Use:  "publish",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		cfg := config.FromContext(ctx)
		mkt := marketplace.NewClient(cfg.GetToken(), cfg.GetMarketplacePrivateEndpoint())

		resp, err := mkt.NewRequest().
			SetPathParam("blueprint_id", blueprintID).
			Post("/blueprints/{blueprint_id}:publish")
		if err != nil {
			logger.FatalCtx(ctx, "publish", zap.Error(err))
		}

		if err = pretty.Print(resp.Body(), cfg.Format); err != nil {
			logger.FatalCtx(ctx, "publish", zap.Error(err))
		}
	},
}
