package cmd

import (
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/pkg/kubectl-saltformula/cmd/get"
	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/pkg/kubectl-saltformula/options"
)

const (
	example = `
  # Get saltformula and watch progress
  %[1]s get bootstrap -w`
)

// NewCmdSaltFormula provides a cobra command wrapping SaltFormulaOptions
func NewCmdSaltFormula(o *options.SaltFormulaOptions) *cobra.Command {
	cmd := &cobra.Command{
		Use:               "kubectl-sf COMMAND",
		Short:             "Manage salt formula",
		Example:           o.Example(example),
		SilenceUsage:      true,
		PersistentPreRunE: o.PersistentPreRunE,
		RunE: func(c *cobra.Command, args []string) error {
			return o.UsageErr(c)
		},
	}
	o.AddKubectlFlags(cmd.PersistentFlags())
	cmd.AddCommand(get.NewCmdGet(o))

	return cmd
}
