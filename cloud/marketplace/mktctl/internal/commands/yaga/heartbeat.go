package yaga

import (
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/logger"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/marketplace"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/pretty"

	"github.com/spf13/cobra"
	"go.uber.org/zap"
)

func init() {
	heartbeatCmd.Flags().StringVar(&instanceID, "instance-id", "", "")

	_ = heartbeatCmd.MarkFlagRequired("instance-id")

	yagaCmd.AddCommand(heartbeatCmd)
}

var heartbeatCmd = &cobra.Command{
	Use:  "heartbeat",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		cfg := config.FromContext(ctx)
		mkt := marketplace.NewClient(cfg.GetToken(), cfg.GetMarketplaceConsoleEndpoint())

		resp, err := mkt.NewRequest().
			SetPathParam("instance_id", instanceID).
			Get("/yaga/{instance_id}/heartbeat")
		if err != nil {
			logger.FatalCtx(ctx, "get", zap.Error(err))
		}

		if err = pretty.Print(resp.Body(), cfg.Format); err != nil {
			logger.FatalCtx(ctx, "get", zap.Error(err))
		}
	},
}
