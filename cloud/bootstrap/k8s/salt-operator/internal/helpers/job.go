package helpers

import (
	kbatch "k8s.io/api/batch/v1"
	corev1 "k8s.io/api/core/v1"
)

func JobStatus(job *kbatch.Job) kbatch.JobConditionType {
	for _, c := range job.Status.Conditions {
		if (c.Type == kbatch.JobComplete || c.Type == kbatch.JobFailed) && c.Status == corev1.ConditionTrue {
			return c.Type
		}
	}
	return ""
}
