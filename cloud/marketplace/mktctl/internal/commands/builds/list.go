package builds

import (
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/logger"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/marketplace"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/pretty"

	"github.com/spf13/cobra"
	"go.uber.org/zap"
)

func init() {
	buildsCmd.AddCommand(list)
}

var list = &cobra.Command{
	Use:  "list",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		cfg := config.FromContext(ctx)
		mkt := marketplace.NewClient(cfg.GetToken(), cfg.GetMarketplacePrivateEndpoint())

		resp, err := mkt.NewRequest().
			Get("/builds")
		if err != nil {
			logger.FatalCtx(ctx, "list", zap.Error(err))
		}

		if err = pretty.Print(resp.Body(), cfg.Format); err != nil {
			logger.FatalCtx(ctx, "list", zap.Error(err))
		}
	},
}
