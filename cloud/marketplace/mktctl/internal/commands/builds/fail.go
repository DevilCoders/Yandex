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
	fail.Flags().StringVar(&buildID, "build-id", "", "")
	_ = fail.MarkFlagRequired("build-id")

	buildsCmd.AddCommand(list)
}

var fail = &cobra.Command{
	Use:  "fail",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		cfg := config.FromContext(ctx)
		mkt := marketplace.NewClient(cfg.GetToken(), cfg.GetMarketplacePrivateEndpoint())

		resp, err := mkt.NewRequest().
			SetPathParam("build_id", buildID).
			Get("/builds/{build_id}:fail")
		if err != nil {
			logger.FatalCtx(ctx, "fail", zap.Error(err))
		}

		if err = pretty.Print(resp.Body(), cfg.Format); err != nil {
			logger.FatalCtx(ctx, "fail", zap.Error(err))
		}
	},
}
