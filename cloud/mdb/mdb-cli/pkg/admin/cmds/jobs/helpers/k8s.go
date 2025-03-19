package helpers

import (
	"time"

	batch "k8s.io/api/batch/v1"
	core "k8s.io/api/core/v1"
	meta "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/tools/clientcmd"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
)

func NewKubernetesAPI(env *cli.Env) *kubernetes.Clientset {
	cfg := config.FromEnv(env)
	kubeCtx := cfg.DeploymentConfig().KubeCtx
	env.L().Debugf("create kube API for context %q", kubeCtx)

	c, err := clientcmd.NewNonInteractiveDeferredLoadingClientConfig(
		clientcmd.NewDefaultClientConfigLoadingRules(),
		&clientcmd.ConfigOverrides{
			CurrentContext: kubeCtx,
		},
	).ClientConfig()
	if err != nil {
		env.L().Fatalf("failed to get kube config: %s", err)
	}

	api, err := kubernetes.NewForConfig(c)
	if err != nil {
		env.L().Fatalf("failed to create kube api: %s", err)
	}

	return api
}

func RunK8sJob(env *cli.Env, name string, podSpec core.PodSpec) {
	cfg := config.FromEnv(env)
	client := NewKubernetesAPI(env)

	namespace := cfg.DeploymentConfig().KubeNamespace
	env.L().Debugf("run job %q in ns %q", name, namespace)
	jobs := client.BatchV1().Jobs(namespace)

	ttl := int32(24 * time.Hour.Seconds())
	var backoff int32
	spec := &batch.Job{
		ObjectMeta: meta.ObjectMeta{
			Name:      name,
			Namespace: namespace,
		},
		Spec: batch.JobSpec{
			BackoffLimit: &backoff,
			Template: core.PodTemplateSpec{
				Spec: podSpec,
			},
			TTLSecondsAfterFinished: &ttl,
		},
	}
	env.L().Debugf("run job with spec %q", spec.String())

	job, err := jobs.Create(env.ShutdownContext(), spec, meta.CreateOptions{})
	if err != nil {
		env.L().Fatalf("can not create job: %s", err)
	}

	env.L().Infof("created job: %q", job.Name)
}

func PodTemplateSpecByStatefulSet(env *cli.Env, statefulSetName string) core.PodSpec {
	client := NewKubernetesAPI(env)
	cfg := config.FromEnv(env)
	namespace := cfg.DeploymentConfig().KubeNamespace

	env.L().Debugf("get StatefulSet with name %q in namespace %q", statefulSetName, namespace)
	api := client.AppsV1().StatefulSets(namespace)
	ss, err := api.Get(env.ShutdownContext(), statefulSetName, meta.GetOptions{})
	if err != nil {
		env.L().Fatalf("can not get StatefulSet: %s", err)
	}

	return ss.Spec.Template.Spec
}

func PodTemplateSpecByDeployment(env *cli.Env, deploymentName string) core.PodSpec {
	client := NewKubernetesAPI(env)
	cfg := config.FromEnv(env)
	namespace := cfg.DeploymentConfig().KubeNamespace

	env.L().Debugf("get Deployment with name %q in namespace %q", deploymentName, namespace)
	api := client.AppsV1().Deployments(namespace)
	ss, err := api.Get(env.ShutdownContext(), deploymentName, meta.GetOptions{})
	if err != nil {
		env.L().Fatalf("can not get Deployment: %s", err)
	}

	return ss.Spec.Template.Spec
}

func JobLogs(env *cli.Env, name string) {
	client := NewKubernetesAPI(env)
	cfg := config.FromEnv(env)
	namespace := cfg.DeploymentConfig().KubeNamespace

	pods, err := client.CoreV1().Pods(namespace).List(env.ShutdownContext(), meta.ListOptions{})
	if err != nil {
		env.L().Fatalf("can not list pods: %s", err)
	}

	var podName string
	for _, pod := range pods.Items {
		if jobName, ok := pod.ObjectMeta.Labels["job-name"]; ok && jobName == name {
			podName = pod.Name
			break
		}
	}

	if podName == "" {
		env.L().Fatalf("can not find pod for job %q", name)
	}

	logs, err := client.CoreV1().Pods(namespace).GetLogs(podName, &core.PodLogOptions{}).Do(env.ShutdownContext()).Raw()
	if err != nil {
		env.L().Fatalf("can not get pod logs for %q: %s", podName, err)
	}

	env.L().Infof("logs: \n%s", string(logs))
}

func ContainerByNameInSpec(spec core.PodSpec, name string) (core.Container, error) {
	for _, container := range spec.Containers {
		if container.Name == name {
			return container, nil
		}
	}
	return core.Container{}, semerr.NotFoundf("container with name %q was not found", name)
}
