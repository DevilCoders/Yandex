package profile

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/logger"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/pretty"

	"github.com/spf13/cobra"
	"go.uber.org/zap"
)

func init() {
	profileCmd.AddCommand(profileGet)
}

var profileGet = &cobra.Command{
	Use:  "get",
	Args: cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		cfg := config.FromContext(cmd.Context())

		profile := cfg.GetProfile(args[0])
		if profile == nil {
			logger.FatalCtx(ctx, "not found")
		}

		data, err := json.Marshal(profile)
		if err != nil {
			logger.FatalCtx(ctx, "struct into json", zap.Error(err))
		}

		if err := pretty.Print(data, cfg.Format); err != nil {
			logger.FatalCtx(ctx, "get", zap.Error(err))
		}
	},
}
