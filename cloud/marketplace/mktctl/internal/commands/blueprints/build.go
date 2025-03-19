package blueprints

import (
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/logger"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/marketplace"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/pretty"

	"github.com/spf13/cobra"
	"go.uber.org/zap"
)

func init() {
	build.Flags().StringVar(&blueprintID, "blueprint-id", "", "")
	_ = build.MarkFlagRequired("blueprint-id")

	blueprintsCmd.AddCommand(build)
}

var build = &cobra.Command{
	Use:  "build",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		cfg := config.FromContext(ctx)
		mkt := marketplace.NewClient(cfg.GetToken(), cfg.GetMarketplacePrivateEndpoint())

		resp, err := mkt.NewRequest().
			SetPathParam("blueprint_id", blueprintID).
			Post("/blueprints/{blueprint_id}:build")
		if err != nil {
			logger.FatalCtx(ctx, "build", zap.Error(err))
		}

		if err = pretty.Print(resp.Body(), cfg.Format); err != nil {
			logger.FatalCtx(ctx, "build", zap.Error(err))
		}
	},
}
