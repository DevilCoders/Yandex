package options

import (
	"context"
	"errors"
	"fmt"

	"github.com/spf13/cobra"
	"github.com/spf13/pflag"
	"k8s.io/cli-runtime/pkg/genericclioptions"
	"k8s.io/client-go/kubernetes/scheme"
	"k8s.io/client-go/tools/clientcmd"
	"sigs.k8s.io/controller-runtime/pkg/client"
	"sigs.k8s.io/controller-runtime/pkg/client/apiutil"
	"sigs.k8s.io/controller-runtime/pkg/manager/signals"

	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/api/v1alpha1"
)

const cliName = "kubectl sf"

// SaltFormulaOptions are a set of common CLI flags and convenience functions made available to
// all commands of the kubectl-sf plugin
type SaltFormulaOptions struct {
	configFlags *genericclioptions.ConfigFlags
	//RESTClientGetter genericclioptions.RESTClientGetter

	genericclioptions.IOStreams
	Client        client.Client
	Ctx           context.Context
	UserNamespace string
}

func (o SaltFormulaOptions) Example(e string) string {
	return fmt.Sprintf(e, cliName)
}

// UsageErr is a convenience function to output usage and return an error
func (o *SaltFormulaOptions) UsageErr(c *cobra.Command) error {
	_ = c.Usage()
	c.SilenceErrors = true
	return errors.New(c.UsageString())
}

func (o SaltFormulaOptions) Clientset() (client.Client, error) {
	if o.Client == nil {
		clientConfig := o.configFlags.ToRawKubeConfigLoader()
		var err error
		o.Client, err = newClient(clientConfig)
		if err != nil {
			return nil, fmt.Errorf("unable to instantiate client, err: %w", err)
		}
	}
	return o.Client, nil
}

func (o SaltFormulaOptions) Namespace(cmd *cobra.Command) (string, error) {
	if o.UserNamespace == "" {
		var err error
		o.UserNamespace, _, err = o.configFlags.ToRawKubeConfigLoader().Namespace()
		if err != nil {
			return "", err
		}

		if o.UserNamespace != "" {
			return o.UserNamespace, nil
		}

		o.UserNamespace, err = cmd.Flags().GetString("namespace")
		if err != nil {
			return "", err
		}
	}

	return o.UserNamespace, nil
}

func (o SaltFormulaOptions) AddKubectlFlags(flags *pflag.FlagSet) {
	o.configFlags.AddFlags(flags)
}

// NewSaltFormulaOptions provides an instance of SaltFormulaOptions with default values
func NewSaltFormulaOptions(streams genericclioptions.IOStreams) *SaltFormulaOptions {
	return &SaltFormulaOptions{
		configFlags: genericclioptions.NewConfigFlags(true),

		IOStreams: streams,
	}
}

// newClient returns new client instance.
func newClient(clientConfig clientcmd.ClientConfig) (client.Client, error) {
	restConfig, err := clientConfig.ClientConfig()
	if err != nil {
		return nil, fmt.Errorf("unable to get rest client config, err: %w", err)
	}

	// Create the mapper provider.
	mapper, err := apiutil.NewDiscoveryRESTMapper(restConfig)
	if err != nil {
		return nil, fmt.Errorf("unable to to instantiate mapper, err: %w", err)
	}

	if err = v1alpha1.AddToScheme(scheme.Scheme); err != nil {
		return nil, fmt.Errorf("unable register SaltFormula apis, err: %w", err)
	}
	// Create the Client for Read/Write operations.
	var newClient client.Client
	newClient, err = client.New(restConfig, client.Options{Scheme: scheme.Scheme, Mapper: mapper})
	if err != nil {
		return nil, fmt.Errorf("unable to instantiate client, err: %w", err)
	}

	return newClient, nil
}

// PersistentPreRunE contains common logic which will be executed for all commands
func (o *SaltFormulaOptions) PersistentPreRunE(c *cobra.Command, args []string) error {
	// NOTE: we set the output of the cobra command to stderr because the only thing that should
	// emit to this are returned errors from command.RunE
	c.SetOut(o.ErrOut)
	c.SetErr(o.ErrOut)

	return nil
}

func (o *SaltFormulaOptions) SetupSignalHandler() context.Context {
	if o.Ctx == nil {
		o.Ctx = signals.SetupSignalHandler()
	}

	return o.Ctx
}
