package helpers

import (
	"fmt"

	"github.com/gofrs/uuid"
	core "k8s.io/api/core/v1"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
)

const (
	workerStatefulSetName = "worker"
	workerContainerName   = "worker"
)

var (
	WorkerBinTpl   = "/opt/yandex/dbaas-worker/%s"
	BaseWorkerArgs = []string{"-c", "/etc/yandex/dbaas-worker/dbaas-worker.conf", "/etc/yandex/dbaas-worker-secrets/dbaas-worker-secrets.conf"}
)

func workerArgs(name string, args []string) []string {
	return append(append([]string{fmt.Sprintf(WorkerBinTpl, name)}, BaseWorkerArgs...), args...)
}

func RunWorkerJob(env *cli.Env, name string, args []string) {
	cmd := workerArgs(name, args)

	podSpec := PodTemplateSpecByStatefulSet(env, workerStatefulSetName)
	podSpec.RestartPolicy = core.RestartPolicyNever

	container, err := ContainerByNameInSpec(podSpec, workerContainerName)
	if err != nil {
		env.L().Fatalf("can not find container in StatefulSet %q: %s", workerStatefulSetName, err)
	}

	var rootUID int64
	container.SecurityContext = &core.SecurityContext{
		RunAsUser: &rootUID,
	}
	container.Command = cmd
	podSpec.Containers = []core.Container{container}
	podSpec.InitContainers = nil

	env.L().Debugf("execute %v", cmd)
	uid, err := uuid.NewV4()
	if err != nil {
		env.L().Fatalf("can not generate uuid: %s", err)
	}
	RunK8sJob(env, fmt.Sprintf("%s-%s", name, uid.String()[:5]), podSpec)
}
