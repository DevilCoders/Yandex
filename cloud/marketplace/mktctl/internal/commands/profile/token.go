package profile

import (
	"fmt"

	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"

	"github.com/spf13/cobra"
)

func init() {
	profileCmd.AddCommand(profileToken)
}

var profileToken = &cobra.Command{
	Use:  "token",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		cfg := config.FromContext(ctx)
		token := cfg.GetToken()

		fmt.Println(token)
	},
}
