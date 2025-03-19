package info

import (
	"fmt"
	"sort"
	"strings"

	corev1 "k8s.io/api/core/v1"
	v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/types"
)

const (
	// nodeUnreachablePodReason is the reason on a pod when its state cannot be confirmed as kubelet is unresponsive
	// on the node it is (was) running.
	// copy-pasted from https://github.com/kubernetes/kubernetes/blob/master/pkg/util/node/node.go#L37 to not add additional dependency
	nodeUnreachablePodReason = "NodeLost"
)

type PodInfo struct {
	*v1.ObjectMeta

	Status string
	Icon   string
}

func addPodInfos(jobInfos []*JobInfo, pods *corev1.PodList) []*JobInfo {
	var uids []types.UID
	uidToJobInfoIdx := make(map[types.UID]int)
	for i, jobInfo := range jobInfos {
		uids = append(uids, jobInfo.ObjectMeta.UID)
		uidToJobInfoIdx[jobInfo.ObjectMeta.UID] = i
	}

	for _, pod := range pods.Items {
		owner := ownerRef(pod.OwnerReferences, uids)
		if owner == nil {
			continue
		}

		podInfo := newPodInfo(&pod)
		idx := uidToJobInfoIdx[owner.UID]
		jobInfos[idx].Pods = append(jobInfos[idx].Pods, &podInfo)
	}

	for _, jobInfo := range jobInfos {
		sort.Slice(jobInfo.Pods[:], func(i, j int) bool {
			if jobInfo.Pods[i].ObjectMeta.CreationTimestamp != jobInfo.Pods[j].ObjectMeta.CreationTimestamp {
				return jobInfo.Pods[i].ObjectMeta.CreationTimestamp.Before(&jobInfo.Pods[j].ObjectMeta.CreationTimestamp)
			}
			return jobInfo.Pods[i].ObjectMeta.Name < jobInfo.Pods[j].ObjectMeta.Name
		})
	}

	return jobInfos
}

func newPodInfo(pod *corev1.Pod) PodInfo {
	podInfo := PodInfo{
		ObjectMeta: &v1.ObjectMeta{
			Name:              pod.Name,
			Namespace:         pod.Namespace,
			CreationTimestamp: pod.CreationTimestamp,
			UID:               pod.UID,
		},
	}
	restarts := 0
	readyContainers := 0

	reason := string(pod.Status.Phase)
	if pod.Status.Reason != "" {
		reason = pod.Status.Reason
	}

	restarts = 0
	hasRunning := false
	for i := len(pod.Status.ContainerStatuses) - 1; i >= 0; i-- {
		container := pod.Status.ContainerStatuses[i]
		restarts += int(container.RestartCount)
		if container.State.Waiting != nil && container.State.Waiting.Reason != "" {
			reason = container.State.Waiting.Reason
		} else if container.State.Terminated != nil && container.State.Terminated.Reason != "" {
			reason = container.State.Terminated.Reason
		} else if container.State.Terminated != nil && container.State.Terminated.Reason == "" {
			if container.State.Terminated.Signal != 0 {
				reason = fmt.Sprintf("Signal:%d", container.State.Terminated.Signal)
			} else {
				reason = fmt.Sprintf("ExitCode:%d", container.State.Terminated.ExitCode)
			}
		} else if container.Ready && container.State.Running != nil {
			hasRunning = true
			readyContainers++
		}

		// change pod status back to "Running" if there is at least one container still reporting as "Running" status
		if reason == "Completed" && hasRunning {
			reason = "Running"
		}
	}

	if pod.DeletionTimestamp != nil && pod.Status.Reason == nodeUnreachablePodReason {
		reason = "Unknown"
	} else if pod.DeletionTimestamp != nil {
		reason = "Terminating"
	}

	podInfo.Status = reason
	podInfo.Icon = podIcon(podInfo.Status)
	return podInfo
}

func podIcon(status string) string {
	if strings.HasPrefix(status, "Init:") {
		return IconProgressing
	}
	if strings.HasPrefix(status, "Signal:") || strings.HasPrefix(status, "ExitCode:") {
		return IconBad
	}
	// See:
	// https://github.com/kubernetes/kubernetes/blob/master/pkg/kubelet/images/types.go
	// https://github.com/kubernetes/kubernetes/blob/master/pkg/kubelet/kuberuntime/kuberuntime_container.go
	// https://github.com/kubernetes/kubernetes/blob/master/pkg/kubelet/container/sync_result.go
	if strings.HasSuffix(status, "Error") || strings.HasPrefix(status, "Err") {
		return IconWarning
	}
	switch status {
	case "Pending", "Terminating", "ContainerCreating":
		return IconProgressing
	case "Running", "Completed":
		return IconOK
	case "Failed", "InvalidImageName", "CrashLoopBackOff":
		return IconBad
	case "ImagePullBackOff", "RegistryUnavailable":
		return IconWarning
	case "Unknown":
		return IconUnknown
	}
	return " "
}
