package commands

import (
	"os"

	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/logger"

	"github.com/spf13/cobra"
	"go.uber.org/zap"
)

var completionCmd = &cobra.Command{
	Use:                   "completion [bash|zsh|fish|powershell]",
	Short:                 "Generate completion script",
	Long:                  "To load completions",
	Example:               "$ mktctl completion bash > $HOME/.config/mktctl/completion.bash.inc && . ~/.bashrc",
	DisableFlagsInUseLine: true,
	ValidArgs:             []string{"bash", "zsh", "fish", "powershell"},
	Args:                  cobra.ExactValidArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		var err error

		switch args[0] {
		case "bash":
			err = cmd.Root().GenBashCompletion(os.Stdout)
		case "zsh":
			err = cmd.Root().GenZshCompletion(os.Stdout)
		case "fish":
			err = cmd.Root().GenFishCompletion(os.Stdout, true)
		case "powershell":
			err = cmd.Root().GenPowerShellCompletionWithDesc(os.Stdout)
		}

		if err != nil {
			logger.FatalCtx(cmd.Context(), "generate", zap.Error(err))
		}
	},
}
