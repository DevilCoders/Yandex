//
// ToDo:
//	* mb no bindings and just pass variables into config, if "", return current ones
//
package commands

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/commands/blueprints"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/commands/builds"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/commands/console"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/commands/operations"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/commands/products"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/commands/profile"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/commands/publishers"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/commands/yaga"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"

	"github.com/spf13/cobra"
)

var rootCmd = &cobra.Command{Use: "mktctl"}

func Execute(ctx context.Context) error {
	cfg := config.FromContext(ctx)

	rootCmd.PersistentFlags().StringVar(&cfg.Current, "profile", "", "use profile")
	rootCmd.PersistentFlags().StringVar(&cfg.TokenOverride, "token", "", "override token")
	rootCmd.PersistentFlags().StringVar(&cfg.LogLevel, "log-level", "", "override default log level {info|debug}")
	rootCmd.PersistentFlags().StringVar(&cfg.Format, "format", "json", "override {json|yaml}")

	blueprints.AddCommands(rootCmd)
	builds.AddCommands(rootCmd)
	operations.AddCommands(rootCmd)
	products.AddCommands(rootCmd)
	profile.AddCommands(rootCmd)
	publishers.AddCommands(rootCmd)
	console.AddCommands(rootCmd)
	yaga.AddCommands(rootCmd)

	rootCmd.AddCommand(completionCmd)

	return rootCmd.ExecuteContext(ctx)
}
