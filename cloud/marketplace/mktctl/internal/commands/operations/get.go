package operations

import (
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/logger"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/marketplace"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/pretty"

	"github.com/spf13/cobra"
	"go.uber.org/zap"
)

func init() {
	get.Flags().StringVar(&operationID, "operation-id", "", "")
	_ = get.MarkFlagRequired("operation-id")

	operationsCmd.AddCommand(get)
}

var get = &cobra.Command{
	Use:  "get",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		cfg := config.FromContext(ctx)
		mkt := marketplace.NewClient(cfg.GetToken(), cfg.GetMarketplacePrivateEndpoint())

		resp, err := mkt.NewRequest().
			SetPathParam("operation_id", operationID).
			Get("/operations/{operation_id}")
		if err != nil {
			logger.FatalCtx(ctx, "get", zap.Error(err))
		}

		if err = pretty.Print(resp.Body(), cfg.Format); err != nil {
			logger.FatalCtx(ctx, "get", zap.Error(err))
		}
	},
}
