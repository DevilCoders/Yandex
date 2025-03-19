package profile

import (
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/logger"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/pretty"

	"github.com/spf13/cobra"
	"go.uber.org/zap"
)

func init() {
	profileCmd.AddCommand(profileCurrent)
}

var profileCurrent = &cobra.Command{
	Use:  "current",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		cfg := config.FromContext(ctx)

		if err := pretty.Print([]byte(cfg.Current), cfg.Format); err != nil {
			logger.FatalCtx(ctx, "current", zap.Error(err))
		}
	},
}
