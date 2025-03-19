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
	getLatestBuild.Flags().StringVar(&blueprintID, "blueprint-id", "", "")
	_ = getLatestBuild.MarkFlagRequired("blueprint-id")

	blueprintsCmd.AddCommand(getLatestBuild)
}

var getLatestBuild = &cobra.Command{
	Use:  "get-latest-build",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		cfg := config.FromContext(ctx)
		mkt := marketplace.NewClient(cfg.GetToken(), cfg.GetMarketplacePrivateEndpoint())

		resp, err := mkt.NewRequest().
			SetPathParam("blueprint_id", blueprintID).
			Get("/blueprints/{blueprint_id}/latestBuild")
		if err != nil {
			logger.FatalCtx(ctx, "get-latest-build", zap.Error(err))
		}

		if err = pretty.Print(resp.Body(), cfg.Format); err != nil {
			logger.FatalCtx(ctx, "get-latest-build", zap.Error(err))
		}
	},
}
