package main

import (
	"os"

	"github.com/spf13/pflag"
	"k8s.io/cli-runtime/pkg/genericclioptions"

	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/pkg/kubectl-saltformula/cmd"
	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/pkg/kubectl-saltformula/options"
)

func main() {
	flags := pflag.NewFlagSet("kubectl-saltformula", pflag.ExitOnError)
	pflag.CommandLine = flags

	o := options.NewSaltFormulaOptions(genericclioptions.IOStreams{In: os.Stdin, Out: os.Stdout, ErrOut: os.Stderr})
	root := cmd.NewCmdSaltFormula(o)
	if err := root.Execute(); err != nil {
		os.Exit(1)
	}
}
