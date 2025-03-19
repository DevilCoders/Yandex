package options

import (
	"context"
	"fmt"

	"k8s.io/cli-runtime/pkg/genericclioptions"
	"k8s.io/client-go/kubernetes/scheme"
	"sigs.k8s.io/controller-runtime/pkg/client"
	"sigs.k8s.io/controller-runtime/pkg/client/fake"

	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/api/v1alpha1"
	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/pkg/kubectl-saltformula/options"
)

// NewFakeSaltFormulaOptions returns a options.SaltFormulaOptions suitable for testing
func NewFakeSaltFormulaOptions(obj ...client.Object) *options.SaltFormulaOptions {
	iostreams, _, _, _ := genericclioptions.NewTestIOStreams()
	o := options.NewSaltFormulaOptions(iostreams)

	if err := v1alpha1.AddToScheme(scheme.Scheme); err != nil {
		panic(fmt.Errorf("unable register SaltFormula apis, err: %w", err))
	}
	o.Client = fake.NewClientBuilder().WithScheme(scheme.Scheme).WithObjects(obj...).Build()
	o.Ctx = context.TODO()
	return o
}
