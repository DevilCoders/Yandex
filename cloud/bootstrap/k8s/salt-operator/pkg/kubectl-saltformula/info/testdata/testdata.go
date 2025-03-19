package testdata

import (
	"encoding/json"
	"io/ioutil"
	"time"

	kbatch "k8s.io/api/batch/v1"
	corev1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"sigs.k8s.io/controller-runtime/pkg/client"

	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/api/v1alpha1"
	"a.yandex-team.ru/library/go/test/yatest"
)

var testDir string

func init() {
	testDir = yatest.SourcePath("cloud/bootstrap/k8s/salt-operator/pkg/kubectl-saltformula/info/testdata")
}

type SaltFormulaObjects struct {
	SaltFormulas []*v1alpha1.SaltFormula
	Jobs         []*kbatch.Job
	Pods         []*corev1.Pod
}

func (r *SaltFormulaObjects) AllObjects() []client.Object {
	var objs []client.Object
	for _, o := range r.SaltFormulas {
		objs = append(objs, o)
	}
	for _, o := range r.Jobs {
		objs = append(objs, o)
	}
	for _, o := range r.Pods {
		objs = append(objs, o)
	}
	return objs
}

func NewSimpleSaltFormula() *SaltFormulaObjects {
	return discoverObjects(testDir + "/simple")
}

func discoverObjects(path string) *SaltFormulaObjects {
	files, err := ioutil.ReadDir(path)
	if err != nil {
		panic(err)
	}
	// we set creation timestamp so that AGE output in CLI can be compared
	aWeekAgo := metav1.NewTime(time.Now().Add(-7 * 24 * time.Hour).Truncate(time.Second))

	var objs SaltFormulaObjects
	for _, file := range files {
		jsonBytes, err := ioutil.ReadFile(path + "/" + file.Name())
		if err != nil {
			panic(err)
		}
		typeMeta := GetTypeMeta(jsonBytes)
		switch typeMeta.Kind {
		case "SaltFormula":
			var sf v1alpha1.SaltFormula
			err = json.Unmarshal(jsonBytes, &sf)
			if err != nil {
				panic(err)
			}
			sf.CreationTimestamp = aWeekAgo
			objs.SaltFormulas = append(objs.SaltFormulas, &sf)
		case "Job":
			var job kbatch.Job
			err = json.Unmarshal(jsonBytes, &job)
			if err != nil {
				panic(err)
			}
			job.CreationTimestamp = aWeekAgo
			objs.Jobs = append(objs.Jobs, &job)
		case "Pod":
			var pod corev1.Pod
			err = json.Unmarshal(jsonBytes, &pod)
			if err != nil {
				panic(err)
			}
			pod.CreationTimestamp = aWeekAgo
			objs.Pods = append(objs.Pods, &pod)
		}
	}
	return &objs
}

func GetTypeMeta(jsonBytes []byte) metav1.TypeMeta {
	type k8sObj struct {
		metav1.TypeMeta `json:",inline"`
	}
	var o k8sObj
	err := json.Unmarshal(jsonBytes, &o)
	if err != nil {
		panic(err)
	}
	return o.TypeMeta
}
