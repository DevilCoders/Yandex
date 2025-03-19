/*
Copyright 2022.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package controllers

import (
	"context"
	"crypto/tls"
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/url"
	"os"
	"runtime"
	"strconv"
	"strings"
	"time"

	"github.com/go-logr/logr"
	"github.com/gofrs/uuid"
	kbatch "k8s.io/api/batch/v1"
	corev1 "k8s.io/api/core/v1"
	apierrors "k8s.io/apimachinery/pkg/api/errors"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	kruntime "k8s.io/apimachinery/pkg/runtime"
	"k8s.io/apimachinery/pkg/types"
	ref "k8s.io/client-go/tools/reference"
	ctrl "sigs.k8s.io/controller-runtime"
	"sigs.k8s.io/controller-runtime/pkg/builder"
	"sigs.k8s.io/controller-runtime/pkg/client"
	"sigs.k8s.io/controller-runtime/pkg/controller"
	"sigs.k8s.io/controller-runtime/pkg/handler"
	ctrllog "sigs.k8s.io/controller-runtime/pkg/log"
	"sigs.k8s.io/controller-runtime/pkg/predicate"
	"sigs.k8s.io/controller-runtime/pkg/reconcile"
	"sigs.k8s.io/controller-runtime/pkg/source"

	bootstrapv1alpha1 "a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/api/v1alpha1"
	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/internal/helpers"
	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/internal/locks"
	ydblock "a.yandex-team.ru/cloud/bootstrap/ydb-lock"
)

// SaltFormulaReconciler reconciles a SaltFormula object
type SaltFormulaReconciler struct {
	client.Client
	Scheme *kruntime.Scheme
	*config

	ydblock locks.Locker
}

// TODO create CRD for config
type config struct {
	bootstrapHostEndpoint string
	saltRunnerClient      string
	saltRunnerSocketDir   string
	saltRunnerSocketName  string
	lockTimeout           time.Duration
}

const (
	bootstrapHost           = "bootstrap.cloud.yandex.net"
	saltRunnerClient        = "cr.yandex/crp136vhf7jvu9cnpvga/saltrunner-client@sha256:8779cd595289c70af82a7ffd8611757688f9a652c9a97b223874aed46b37fa67"
	saltRunnerSocketDir     = "/var/run/saltrunner"
	saltRunnerSocketName    = "grpc.sock"
	jobOwnerKey             = ".metadata.controller"
	ControllerName          = "SaltFormula"
	hostnameLabel           = "kubernetes.io/hostname"
	scheduledTimeAnnotation = "bootstrap.cloud.yandex.net/scheduled-at"
	bsIDAnnotation          = "bootstrap.cloud.yandex.net/yc-bs-id"
	lockAnnotation          = "bootstrap.cloud.yandex.net/lock"
	requeueAfter            = time.Second * 60
	defaultLockTimeout      = time.Minute * 5
)

var (
	defaultMaxConcurrentReconciles = runtime.NumCPU()
)

func useConstIfEmpty(key string, def string) (res string) {
	res = os.Getenv(key)
	if res == "" {
		res = def
	}
	return res
}

func NewSaltFormulaReconciler(mgr ctrl.Manager, ydblock locks.Locker) *SaltFormulaReconciler {
	lockTimeout, err := time.ParseDuration(useConstIfEmpty("LOCK_TIMEOUT", defaultLockTimeout.String()))
	if err != nil {
		panic(err)
	}
	if lockTimeout.Truncate(time.Second) != lockTimeout {
		panic("LOCK_TIMEOUT does not support resolution less then a second")
	}

	return &SaltFormulaReconciler{
		Client: mgr.GetClient(),
		Scheme: mgr.GetScheme(),
		config: &config{
			bootstrapHostEndpoint: useConstIfEmpty("BOOTSTRAP_HOST", bootstrapHost),
			saltRunnerClient:      useConstIfEmpty("SALT_RUNNER_CLIENT", saltRunnerClient),
			saltRunnerSocketDir:   useConstIfEmpty("SALT_RUNNER_SOCKET_DIR", saltRunnerSocketDir),
			saltRunnerSocketName:  useConstIfEmpty("SALT_RUNNER_SOCKET_NAME", saltRunnerSocketName),
			lockTimeout:           lockTimeout,
		},
		ydblock: ydblock,
	}
}

//+kubebuilder:rbac:groups=bootstrap.cloud.yandex.net,resources=saltformulas,verbs=get;list;watch;create;update;patch;delete
//+kubebuilder:rbac:groups=bootstrap.cloud.yandex.net,resources=saltformulas/status,verbs=get;update;patch
//+kubebuilder:rbac:groups=bootstrap.cloud.yandex.net,resources=saltformulas/finalizers,verbs=update
//+kubebuilder:rbac:groups="",resources=nodes,verbs=get;list;watch
//+kubebuilder:rbac:groups=batch,resources=jobs,verbs=get;list;watch;create;update;patch;delete

// Reconcile is part of the main kubernetes reconciliation loop which aims to
// move the current state of the cluster closer to the desired state.
// For more details, check Reconcile and its Result here:
// - https://pkg.go.dev/sigs.k8s.io/controller-runtime@v0.10.0/pkg/reconcile
func (r *SaltFormulaReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
	log := ctrllog.FromContext(ctx)
	log.WithValues("SaltFormula.Namespace", req.Namespace, "SaltFormula.Name", req.Name)
	log.Info("Reconciling SaltFormula")
	// Fetch the Saltformula instance
	sf := &bootstrapv1alpha1.SaltFormula{}
	err := r.Get(ctx, req.NamespacedName, sf)
	if err != nil {
		if apierrors.IsNotFound(err) {
			// Request object not found, could have been deleted after reconcile request.
			// Owned objects are automatically garbage collected. For additional cleanup logic use finalizers.
			// Return and don't requeue
			log.Info("Saltformula resources not found. Ignoring since object must be deleted")
			return ctrl.Result{}, nil
		}
		// Error reading the object - requeue the request.
		log.Error(err, "Failed to get SaltFormula")
		return ctrl.Result{}, err
	}
	nodes, err := r.getNodesBySelector(ctx, sf.Spec.NodeSelector)
	if err != nil {
		if apierrors.IsNotFound(err) {
			log.Error(err, fmt.Sprintf("Could not find suitable worker nodes. Requeue after %d", requeueAfter), "SaltFormula.BaseRole", sf.Spec.BaseRole, "SaltFormula.NodeSelector", sf.Spec.NodeSelector)
			return ctrl.Result{RequeueAfter: requeueAfter}, nil
		}
		// Error reading the object - requeue the request.
		log.Error(err, "Failed to get NodeList")
		return ctrl.Result{}, err
	}

	nodeSize := int32(len(nodes))
	if sf.Status.Desired != nodeSize {
		sf.Status.Desired = nodeSize
		err = r.Status().Update(ctx, sf)
		if err != nil {
			log.Error(err, "Failed to update salt formula status")
			return ctrl.Result{}, err
		}
	}

	UUID, err := uuid.NewV4()
	if err != nil {
		log.Error(err, "failed to generate UUID")
		return ctrl.Result{RequeueAfter: requeueAfter}, err
	}
	jobID := UUID.String()

	// Check if the Jobs already exists, if not create
	childJobs := &kbatch.JobList{}
	if err := r.List(ctx, childJobs, client.InNamespace(req.Namespace), client.MatchingFields{jobOwnerKey: req.Name}, client.MatchingLabels{"salt_formula_version": sf.Spec.Version}); err != nil {
		log.Error(err, "unable to list child Jobs")
		return ctrl.Result{}, err
	}
	if len(childJobs.Items) > 0 {
		var jobsName []string
		for _, job := range childJobs.Items {
			jobsName = append(jobsName, job.Name)
		}
		log.V(1).Info("saltformula child jobs", "jobs", jobsName)
	}
	/*
		We consider a job "finished" if it has a "Complete" or "Failed" condition marked as true.
		Status conditions allow us to add extensible status information to our objects that other
		humans and controllers can examine to check things like completion and health.
	*/

	// find the active list of jobs
	var activeJobs []*kbatch.Job
	var successfulJobs []*kbatch.Job
	var failedJobs []*kbatch.Job
	var missedNodes []*corev1.Node
	var mostRecentTime *time.Time // find the last run so we can update the status

	getScheduledTimeForJob := func(job *kbatch.Job) (*time.Time, error) {
		timeRaw := job.Annotations[scheduledTimeAnnotation]
		if len(timeRaw) == 0 {
			return nil, nil
		}

		timeParsed, err := time.Parse(time.RFC3339, timeRaw)
		if err != nil {
			return nil, err
		}
		return &timeParsed, nil
	}

	// validate that we have node and job for each
loopNode:
	for _, node := range nodes {
		for _, job := range childJobs.Items {
			if node.Name == job.Spec.Template.Spec.NodeSelector[hostnameLabel] {
				continue loopNode
			}
		}
		missedNodes = append(missedNodes, node)
	}
	if len(missedNodes) > 0 {
		var nodeNames []string
		for _, node := range missedNodes {
			nodeNames = append(nodeNames, node.Name)
		}
		log.V(1).Info("nodes waiting for new jobs", "nodes", nodeNames)
	}

	for i, job := range childJobs.Items {
		switch helpers.JobStatus(&job) {
		case "": // ongoing
			activeJobs = append(activeJobs, &childJobs.Items[i])
			if !sf.Spec.SkipLockUpdatedHosts {
				err = r.extendLockForJob(ctx, &job, sf, log)
				if err != nil {
					log.Info("canceling job because we cannot extend it's lock", "job", job)
					err = r.Delete(ctx, &job)
					if err != nil {
						log.Error(err, "cannot delete job", "job", job)
						continue
					}
				}
			}
		case kbatch.JobFailed:
			failedJobs = append(failedJobs, &childJobs.Items[i])
			r.releaseLockForJob(ctx, &job, log)
		case kbatch.JobComplete:
			successfulJobs = append(successfulJobs, &childJobs.Items[i])
			r.releaseLockForJob(ctx, &job, log)
		}
		scheduledTimeForJob, err := getScheduledTimeForJob(&job)
		if err != nil {
			log.Error(err, "unable to parse scheduled time for child job", "job", &job)
			continue
		}
		if scheduledTimeForJob != nil {
			if mostRecentTime == nil {
				mostRecentTime = scheduledTimeForJob
				jobID = job.Annotations[bsIDAnnotation]
			} else if mostRecentTime.Before(*scheduledTimeForJob) {
				mostRecentTime = scheduledTimeForJob
				jobID = job.Annotations[bsIDAnnotation]
			}
		}
	}
	sf.Status.LastScheduledTime = nil
	if mostRecentTime != nil {
		sf.Status.LastScheduledTime = &metav1.Time{Time: *mostRecentTime}
	}

	sf.Status.Active = nil
	for _, activeJob := range activeJobs {
		jobRef, err := ref.GetReference(r.Scheme, activeJob)
		if err != nil {
			log.Error(err, "unable to make reference to active job", "job", activeJob)
			continue
		}
		sf.Status.Active = append(sf.Status.Active, *jobRef)
	}
	if sf.Status.Epoch == "" || sf.Status.Desired != int32(len(nodes)) {
		// get new epoch only if number of nodes has changed
		masterHost := os.Getenv("KUBERNETES_SERVICE_HOST")
		var epoch string
		if strings.Contains(masterHost, "kten") {
			// if doesnt contain kten, so we are in dev environment, cloudvm with kind, where we couldnt update cluster map
			standName := strings.Split(masterHost, "-")[0]
			epoch, err = r.getStandEpoch(ctx, standName)
			if err != nil {
				log.Error(err, "unable to get stand epoch")
				return ctrl.Result{RequeueAfter: requeueAfter}, err
			}
		}
		sf.Status.Epoch = epoch
	}
	sf.Status.Desired = int32(len(nodes))
	sf.Status.Current = int32(len(failedJobs) + len(successfulJobs))
	sf.Status.Ready = int32(len(successfulJobs))

	log.V(1).Info("job count", "active jobs", len(activeJobs), "successful jobs", len(successfulJobs), "failed jobs", len(failedJobs))
	if err := r.Status().Update(ctx, sf); err != nil {
		log.Error(err, "unable to update SaltFormula status")
		return ctrl.Result{}, err
	}

	// TODO: cleanup old jobs
	// TODO: cleanup parentless jobs, job for nonexisting worker node

	if sf.Spec.Suspend {
		log.Info("saltformula suspended, skipping")
		return ctrl.Result{}, nil
	}

	if len(missedNodes) == 0 && len(activeJobs) == 0 && len(nodes) == (len(successfulJobs)+len(failedJobs)) {
		log.Info("all jobs done")
		return ctrl.Result{}, nil
	}

	if len(activeJobs) >= int(sf.Spec.BatchSize) {
		log.V(1).Info("got max concurrent jobs, requeue after 10s")
		return ctrl.Result{RequeueAfter: requeueAfter}, nil
	}

	// Build job for missed nodes, limits by batchsize
	for i, node := range missedNodes {
		if sf.Spec.MaxFail != 0 && len(failedJobs) >= int(sf.Spec.MaxFail) {
			log.V(1).Info("got max failed jobs, stop scheduling new jobs")
			return ctrl.Result{}, nil
		}

		if i >= int(sf.Spec.BatchSize) {
			log.V(1).Info("got max concurrent jobs, requeue after 10s")
			return ctrl.Result{RequeueAfter: requeueAfter}, nil
		}

		job, err := r.jobForSaltFormula(sf, node, time.Now(), jobID)
		if err != nil {
			log.Error(err, "unable to construct job", "jobID", jobID, "node", node)
			continue
		}

		var lock *ydblock.Lock
		if !sf.Spec.SkipLockUpdatedHosts {
			lock, err = r.createLockForJob(ctx, job, sf)
			if err != nil {
				log.Error(err, "unable to create lock", "job", job)
				continue
			}
			log.V(1).Info("created lock", "job", job, "lock", lock)
			err = updateLockAnnotation(job, lock)
			if err != nil {
				log.Error(err, "failed to marshal lock", "lock", lock)
				continue
			}
		}
		if err := r.Create(ctx, job); err != nil {
			log.Error(err, "unable to create Job for SaltFormula", "job", job)
			if lock != nil {
				lockErr := r.ydblock.ReleaseLock(ctx, lock)
				if lockErr != nil {
					log.Error(lockErr, "unable to release lock", "job", job, "lock", lock)
				}
			}
			return ctrl.Result{}, err
		}
		log.V(1).Info("created Job for SaltFormula run", "job", job)
	}
	log.V(1).Info("all job provisioned, wait completion")
	return ctrl.Result{RequeueAfter: requeueAfter}, err
}

func (r *SaltFormulaReconciler) createLockForJob(ctx context.Context, job *kbatch.Job, sf *bootstrapv1alpha1.SaltFormula) (*ydblock.Lock, error) {
	jobID := job.Annotations[bsIDAnnotation]
	nodeName, ok := job.Spec.Template.Spec.NodeSelector[hostnameLabel]
	if !ok {
		return nil, errors.New("cannot find node for job")
	}
	filter, err := filterForSaltFormula(sf)
	if err != nil {
		return nil, err
	}
	lockDescription := fmt.Sprintf("Updating via salt-operator with filter <%s> (jobID=%s)", filter, jobID)
	return r.ydblock.CreateLock(ctx, lockDescription, []string{nodeName}, uint64(r.config.lockTimeout.Seconds()), ydblock.HostLockTypeOther)
}

func (r *SaltFormulaReconciler) releaseLockForJob(ctx context.Context, job *kbatch.Job, log logr.Logger) {
	lock, err := lockFromJob(job)
	if err != nil {
		log.Error(err, "failed to get lock for job", "job", job)
		return
	}
	err = r.ydblock.ReleaseLock(ctx, lock)
	if err != nil {
		log.Error(err, "failed to release lock", "job", job, "lock", lock)
	}
}

func (r *SaltFormulaReconciler) extendLockForJob(ctx context.Context, job *kbatch.Job, sf *bootstrapv1alpha1.SaltFormula, log logr.Logger) error {
	var extendedLock *ydblock.Lock
	lock, err := lockFromJob(job)
	if err != nil {
		log.Error(err, "failed to get lock for job", "job", job)
		// Try to create new lock in case annotation is missing or corrupted
		extendedLock, err = r.createLockForJob(ctx, job, sf)
		if err != nil {
			log.Error(err, "failed to create new lock", "job", job, "lock", lock)
			return err
		}
	} else {
		extendedLock, err = r.ydblock.ExtendLock(ctx, lock)
		if err != nil {
			log.Error(err, "failed to extend lock", "job", job, "lock", lock)
			// Lock may be already expired if we didn't reconcile for more then r.config.lockTimeout or failed to update
			// the lock object in annotations in previous reconciles, so try to create new lock
			extendedLock, err = r.createLockForJob(ctx, job, sf)
			if err != nil {
				log.Error(err, "failed to create new lock", "job", job, "lock", lock)
				return err
			}
		}
	}
	if err = updateLockAnnotation(job, extendedLock); err != nil {
		log.Error(err, "failed to update lock annotation", "lock", extendedLock)
		return err
	}
	if err = r.Update(ctx, job); err != nil {
		log.Error(err, "failed to update lock in job", "lock", extendedLock, "job", job)
	}
	return nil
}

func updateLockAnnotation(job *kbatch.Job, extendedLock *ydblock.Lock) error {
	jsonLock, err := json.Marshal(extendedLock)
	if err != nil {
		return err
	}
	if job.Annotations == nil {
		job.Annotations = map[string]string{}
	}
	job.Annotations[lockAnnotation] = string(jsonLock)
	return nil
}

// jobForSaltFormula returns a saltformula Job object
func (r *SaltFormulaReconciler) jobForSaltFormula(sf *bootstrapv1alpha1.SaltFormula, node *corev1.Node, scheduledTime time.Time, id string) (*kbatch.Job, error) {
	ls := LabelsForSaltFormula(sf.Name, sf.Spec.BaseRole, sf.Spec.Role, sf.Spec.Version)
	hostPathDirectory := corev1.HostPathDirectory
	automountSAToken := false

	// We want job names for a given worker node to have a deterministic name to avoid the same job being created twice
	shortNodeName := strings.Split(node.Name, ".")[0]
	name := fmt.Sprintf("%s-%s-%s", shortNodeName, sf.Spec.Role, sf.Spec.Version)

	nodeSelector := make(map[string]string)
	for k, v := range sf.Spec.NodeSelector {
		nodeSelector[k] = v
	}
	nodeSelector[hostnameLabel] = node.Name
	job := &kbatch.Job{
		ObjectMeta: metav1.ObjectMeta{
			Labels: ls,
			Annotations: map[string]string{
				scheduledTimeAnnotation: scheduledTime.Format(time.RFC3339),
				bsIDAnnotation:          id,
			},
			Name:      name,
			Namespace: sf.Namespace,
		},
		Spec: kbatch.JobSpec{
			BackoffLimit: &sf.Spec.Retry,
			Template: corev1.PodTemplateSpec{
				ObjectMeta: metav1.ObjectMeta{
					Labels: ls,
				},
				Spec: corev1.PodSpec{
					AutomountServiceAccountToken: &automountSAToken,
					RestartPolicy:                corev1.RestartPolicyNever,
					DNSPolicy:                    corev1.DNSDefault,
					Containers: []corev1.Container{{
						Image:           r.config.saltRunnerClient,
						Name:            "salt-deploy",
						ImagePullPolicy: corev1.PullIfNotPresent,
						Command:         []string{"/saltrunner-client"},
						Args: []string{"run-salt",
							"--server-uds-socket", strings.Join([]string{r.config.saltRunnerSocketDir, r.config.saltRunnerSocketName}, "/"),
							"--bs-id", id,
							"--salt-role", sf.Spec.Role,
							"--salt-formula-package-version", sf.Spec.Version,
							"--apply", strconv.FormatBool(sf.Spec.Apply),
							"--epoch", sf.Status.Epoch,
							"--timeout", "1800",
						},
						VolumeMounts: []corev1.VolumeMount{
							{
								Name:      "socket-volume",
								ReadOnly:  false,
								MountPath: r.config.saltRunnerSocketDir,
							},
						},
					}},
					Volumes: []corev1.Volume{
						{
							Name: "socket-volume",
							VolumeSource: corev1.VolumeSource{
								HostPath: &corev1.HostPathVolumeSource{
									Path: r.config.saltRunnerSocketDir,
									Type: &hostPathDirectory,
								},
							},
						},
					},
					NodeSelector: nodeSelector,
				},
			},
		},
	}
	if err := ctrl.SetControllerReference(sf, job, r.Scheme); err != nil {
		return nil, err
	}

	return job, nil
}

// LabelsForSaltFormula returns the labels for selecting the resources
// belonging to the given salt-formula CR name.
func LabelsForSaltFormula(name string, baseRole string, role string, version string) map[string]string {
	return map[string]string{
		"app":                    "salt-formula",
		"salt_formula_cr":        name,
		"salt_formula_base_role": baseRole,
		"salt_formula_role":      role,
		"salt_formula_version":   version,
	}
}

// SetupWithManager sets up the controller with the Manager.
func (r *SaltFormulaReconciler) SetupWithManager(mgr ctrl.Manager) error {
	if err := mgr.GetFieldIndexer().IndexField(context.Background(), &kbatch.Job{}, jobOwnerKey, func(rawObj client.Object) []string {
		// grab the job object, extract the owner...
		job := rawObj.(*kbatch.Job)
		owner := metav1.GetControllerOf(job)
		if owner == nil {
			return nil
		}
		// ...make sure it's a SaltFormula...
		if owner.Kind != ControllerName {
			return nil
		}

		// ...and if so, return it
		return []string{owner.Name}
	}); err != nil {
		return err
	}

	if err := ctrl.NewControllerManagedBy(mgr).
		For(&bootstrapv1alpha1.SaltFormula{}).
		Owns(&kbatch.Job{}).
		Watches(
			&source.Kind{Type: &corev1.Node{}},
			handler.EnqueueRequestsFromMapFunc(r.isSuitableNode),
			builder.WithPredicates(predicate.LabelChangedPredicate{}),
		).
		WithOptions(controller.Options{MaxConcurrentReconciles: defaultMaxConcurrentReconciles}).
		Complete(r); err != nil {
		return err
	}
	return nil
}

func (r *SaltFormulaReconciler) isSuitableNode(nodeObject client.Object) []reconcile.Request {
	sfObjects := &bootstrapv1alpha1.SaltFormulaList{}
	_ = r.List(context.TODO(), sfObjects)
	requests := []reconcile.Request{}
	for _, sfObject := range sfObjects.Items {
		if len(sfObject.Spec.NodeSelector) == 0 || !checkNodeSelectorMatches(nodeObject.GetLabels(), sfObject.Spec.NodeSelector) {
			continue
		}

		requests = append(requests, reconcile.Request{
			NamespacedName: types.NamespacedName{
				Name:      sfObject.Name,
				Namespace: sfObject.Namespace,
			},
		})
	}
	return requests
}

func checkNodeSelectorMatches(nodeLabels map[string]string, nodeSelector map[string]string) bool {
	for label, value := range nodeSelector {
		if nodeLabel, ok := nodeLabels[label]; !ok || value != nodeLabel {
			return false
		}
	}
	return true
}

// getNodesBySelector return nodeList which are filtered by node selector and have status running
func (r *SaltFormulaReconciler) getNodesBySelector(ctx context.Context, nodeSelector map[string]string) (availableNodes []*corev1.Node, err error) {
	log := ctrllog.FromContext(ctx)

	if len(nodeSelector) == 0 {
		return nil, errors.New("empty nodeSelector is not allowed")
	}

	matchingLabels := client.MatchingLabels{}
	for k, v := range nodeSelector {
		matchingLabels[k] = v
	}
	// list nodes where role could be located
	nodeList := &corev1.NodeList{}
	nodeListOpts := []client.ListOption{
		matchingLabels,
	}
	err = r.List(ctx, nodeList, nodeListOpts...)
	if err != nil {
		return nil, err
	}
	for i, n := range nodeList.Items {
		for _, condition := range n.Status.Conditions {
			if condition.Type == corev1.NodeReady && condition.Status == corev1.ConditionTrue {
				log.V(1).Info("add available node", "nodeName", n.Name)
				availableNodes = append(availableNodes, &nodeList.Items[i])
				break
			}
		}
	}
	return availableNodes, err
}

func (r *SaltFormulaReconciler) getStandEpoch(ctx context.Context, stand string) (string, error) {
	bootstrapURL, err := url.Parse(fmt.Sprintf("https://%s", r.config.bootstrapHostEndpoint))
	if err != nil {
		return "", err
	}
	bootstrapURL.Path = "/v1/epoch"
	req, err := http.NewRequestWithContext(ctx, http.MethodGet, bootstrapURL.String(), nil)
	if err != nil {
		return "", err
	}
	tlsConfig := &tls.Config{
		InsecureSkipVerify: true,
	}
	transport := &http.Transport{TLSClientConfig: tlsConfig}
	cl := &http.Client{Transport: transport}
	resp, err := cl.Do(req)
	if err != nil {
		return "", err
	}
	epoch, _ := ioutil.ReadAll(resp.Body)
	defer resp.Body.Close()
	bootstrapURL.Path = fmt.Sprintf("/v1/%s/%s/cluster_map/grains.epoch", strings.TrimSuffix(string(epoch), "\n"), stand)
	req, err = http.NewRequestWithContext(ctx, http.MethodGet, bootstrapURL.String(), nil)
	if err != nil {
		return "", err
	}
	resp, err = cl.Do(req)
	if err != nil {
		return "", err
	}
	epochStand, _ := ioutil.ReadAll(resp.Body)
	defer resp.Body.Close()
	return strings.TrimSuffix(string(epochStand), "\n"), nil
}

func lockFromJob(job *kbatch.Job) (lock *ydblock.Lock, err error) {
	jsonLock, ok := job.Annotations[lockAnnotation]
	if !ok {
		return nil, fmt.Errorf("cannot find annotation %s on job %v", lockAnnotation, job)
	}

	err = json.Unmarshal([]byte(jsonLock), &lock)
	if err != nil {
		return nil, fmt.Errorf("failed to unmarshal lock from annotation: %w", err)
	}

	return lock, nil
}

func filterForSaltFormula(sf *bootstrapv1alpha1.SaltFormula) (string, error) {
	var filterParts []string
	filterParts = append(filterParts, fmt.Sprintf("salt_roles=%s", sf.Spec.Role), fmt.Sprintf("roles=%s", sf.Spec.BaseRole))
	k8sLabels, err := json.Marshal(sf.Spec.NodeSelector)
	if err != nil {
		return "", err
	}
	filterParts = append(filterParts, fmt.Sprintf("k8s_labels=%s", k8sLabels))

	return strings.Join(filterParts, " "), nil
}
