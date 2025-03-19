package info

import (
	kbatch "k8s.io/api/batch/v1"
	corev1 "k8s.io/api/core/v1"
	v1 "k8s.io/apimachinery/pkg/apis/meta/v1"

	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/api/v1alpha1"
)

type SaltFormulaInfo struct {
	*v1.ObjectMeta

	Jobs              []*JobInfo
	BaseRole          string
	Role              string
	Desired           int32
	Current           int32
	Ready             int32
	Epoch             string
	LastScheduledTime *v1.Time
}

func NewSaltFormulaInfo(
	sf *v1alpha1.SaltFormula,
	jobs *kbatch.JobList,
	pods *corev1.PodList,
) *SaltFormulaInfo {

	sfInfo := SaltFormulaInfo{
		ObjectMeta: &v1.ObjectMeta{
			Name:              sf.Name,
			Namespace:         sf.Namespace,
			UID:               sf.UID,
			CreationTimestamp: sf.CreationTimestamp,
			ResourceVersion:   sf.ObjectMeta.ResourceVersion,
		},
	}

	sfInfo.Jobs = getJobInfo(sf.UID, jobs, pods)

	sfInfo.BaseRole = sf.Spec.BaseRole
	sfInfo.Role = sf.Spec.Role

	sfInfo.Desired = sf.Status.Desired
	sfInfo.Current = sf.Status.Current
	sfInfo.Ready = sf.Status.Ready
	sfInfo.Epoch = sf.Status.Epoch

	sfInfo.LastScheduledTime = sf.Status.LastScheduledTime
	return &sfInfo
}
