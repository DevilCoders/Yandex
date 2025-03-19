package helpers

import (
	"fmt"
	"strings"

	"github.com/gofrs/uuid"
	core "k8s.io/api/core/v1"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
)

const (
	backupWorkerCliDeploymentName = "backup-worker-cli"
	backupWorkerCliContainerName  = "backup-worker-cli"
)

var (
	BackupWorkerCliBin      = "/opt/yandex/backup-worker-cli/backup-worker-cli"
	BaseBackupWorkerCliArgs = []string{"--config-path", "/etc/yandex/backup-worker-cli/"}
)

func backupWorkerArgs(name string, args []string) []string {
	return append(append([]string{BackupWorkerCliBin, name}, BaseBackupWorkerCliArgs...), args...)
}

func RunBackupWorkerJob(env *cli.Env, name string, args []string) {
	cmd := backupWorkerArgs(name, args)

	podSpec := PodTemplateSpecByDeployment(env, backupWorkerCliDeploymentName)
	podSpec.RestartPolicy = core.RestartPolicyNever

	container, err := ContainerByNameInSpec(podSpec, backupWorkerCliContainerName)
	if err != nil {
		env.L().Fatalf("can not find container in Deployment %q: %s", backupWorkerCliDeploymentName, err)
	}

	container.Command = cmd
	podSpec.Containers = []core.Container{container}

	env.L().Debugf("execute %v", cmd)
	uid, err := uuid.NewV4()
	if err != nil {
		env.L().Fatalf("can not generate uuid: %s", err)
	}

	RunK8sJob(env, fmt.Sprintf("%s-%s", strings.ReplaceAll(name, "_", "-"), uid.String()[:5]), podSpec)
}
