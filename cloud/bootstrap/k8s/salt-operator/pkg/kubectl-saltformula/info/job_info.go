package info

import (
	"sort"

	kbatch "k8s.io/api/batch/v1"
	corev1 "k8s.io/api/core/v1"
	v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/types"

	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/internal/helpers"
)

type JobInfo struct {
	*v1.ObjectMeta

	Status         string
	StartTime      *v1.Time
	CompletionTime *v1.Time
	Icon           string
	Pods           []*PodInfo
}

func getJobInfo(ownerUID types.UID, jobs *kbatch.JobList, pods *corev1.PodList) []*JobInfo {
	var jobInfos []*JobInfo
	for _, job := range jobs.Items {
		// if owned by saltformula
		if ownerRef(job.OwnerReferences, []types.UID{ownerUID}) == nil {
			continue
		}

		jobInfo := &JobInfo{
			ObjectMeta: &v1.ObjectMeta{
				Name:              job.Name,
				Namespace:         job.Namespace,
				CreationTimestamp: job.CreationTimestamp,
				UID:               job.UID,
			},
			Status:         getJobStatus(job),
			StartTime:      job.Status.StartTime,
			CompletionTime: job.Status.CompletionTime,
		}

		jobInfo.Icon = jobIcon(jobInfo.Status)

		jobInfos = append(jobInfos, jobInfo)
	}
	sort.Slice(jobInfos[:], func(i, j int) bool {
		if jobInfos[i].ObjectMeta.CreationTimestamp != jobInfos[j].ObjectMeta.CreationTimestamp {
			jobInfos[i].ObjectMeta.CreationTimestamp.Before(&jobInfos[j].ObjectMeta.CreationTimestamp)
		}
		return jobInfos[i].ObjectMeta.Name < jobInfos[j].ObjectMeta.Name
	})
	return addPodInfos(jobInfos, pods)
}

func jobIcon(status string) string {
	switch status {
	case "Running":
		return IconProgressing
	case "Complete":
		return IconOK
	case "Failed":
		return IconBad
	case "Suspended":
		return IconWarning
	}
	return " "
}

func getJobStatus(job kbatch.Job) string {
	status := helpers.JobStatus(&job)
	if status == "" {
		return "Running"
	}
	return string(status)
}
