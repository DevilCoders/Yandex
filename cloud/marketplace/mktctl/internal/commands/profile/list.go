package profile

import (
	"fmt"

	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"

	"github.com/spf13/cobra"
)

func init() {
	profileCmd.AddCommand(profileList)
}

var profileList = &cobra.Command{
	Use:  "list",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		cfg := config.FromContext(cmd.Context())

		for k := range cfg.Profiles {
			if k == cfg.Current {
				fmt.Printf("  * %v\n", k)
			} else {
				fmt.Printf("    %v\n", k)
			}
		}
	},
}
