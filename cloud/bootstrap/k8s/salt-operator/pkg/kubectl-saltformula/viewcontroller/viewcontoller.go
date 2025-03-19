package viewcontroller

import (
	"context"

	kbatch "k8s.io/api/batch/v1"
	v1 "k8s.io/api/core/v1"
	"sigs.k8s.io/controller-runtime/pkg/client"

	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/api/v1alpha1"
	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/controllers"
	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/pkg/kubectl-saltformula/info"
)

// viewController is a mini controller which allows printing of live updates to salt formula
// TODO(syndicut): Implement actual watch logics
// Allows subscribers to receive updates about
type viewController struct {
	name      string
	namespace string

	client client.Reader

	getObj func(context.Context) (interface{}, error)
}

type SaltFormulaViewController struct {
	*viewController
}

func NewSaltFormulaViewController(namespace string, name string, kubeClient client.Reader) *SaltFormulaViewController {
	vc := newViewController(namespace, name, kubeClient)
	rvc := SaltFormulaViewController{
		viewController: vc,
	}
	vc.getObj = func(ctx context.Context) (interface{}, error) {
		return rvc.GetSaltFormulaInfo(ctx)
	}
	return &rvc
}

func newViewController(namespace string, name string, kubeClient client.Reader) *viewController {
	controller := viewController{
		name:      name,
		namespace: namespace,
		client:    kubeClient,
	}

	return &controller
}

func (c *SaltFormulaViewController) GetSaltFormulaInfo(ctx context.Context) (*info.SaltFormulaInfo, error) {
	sf := &v1alpha1.SaltFormula{}
	err := c.client.Get(ctx, client.ObjectKey{Namespace: c.namespace, Name: c.name}, sf)
	if err != nil {
		return nil, err
	}

	jobs := &kbatch.JobList{}
	err = c.client.List(ctx, jobs, client.InNamespace(c.namespace), client.MatchingLabels(controllers.LabelsForSaltFormula(sf.Name, sf.Spec.BaseRole, sf.Spec.Role, sf.Spec.Version)))
	if err != nil {
		return nil, err
	}

	pods := &v1.PodList{}
	err = c.client.List(ctx, pods, client.InNamespace(c.namespace), client.MatchingLabels(controllers.LabelsForSaltFormula(sf.Name, sf.Spec.BaseRole, sf.Spec.Role, sf.Spec.Version)))
	if err != nil {
		return nil, err
	}

	roInfo := info.NewSaltFormulaInfo(sf, jobs, pods)
	return roInfo, nil
}
