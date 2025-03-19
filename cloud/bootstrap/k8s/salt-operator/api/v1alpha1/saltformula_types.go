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

package v1alpha1

import (
	corev1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

// NOTE: json tags are required.  Any new fields you add must have json tags for the fields to be serialized.

// SaltFormulaSpec defines the desired state of SaltFormula
type SaltFormulaSpec struct {
	// BaseRole defines base role for which this salt-formula version should be applied
	BaseRole string `json:"baseRole"`

	// Role is salt formula role for update
	Role string `json:"role"`

	// Version is deb package version of yc-salt-formula
	Version string `json:"version"`

	// BatchSize is how many jobs, salt-call per node, will be running at the same time
	// +kubebuilder:default:=1
	BatchSize uint8 `json:"batchSize,omitempty"`

	// MaxFail is how many jobs can fail before we stop scheduling new jobs
	MaxFail uint8 `json:"maxFail,omitempty"`

	// Retry using as BackoffLimit in JobSpec
	// Specifies the number of retries before marking this job failed.
	// +kubebuilder:default:=1
	Retry int32 `json:"retry,omitempty"`

	// +kubebuilder:default:=false
	// Apply run salt-call without test=True
	Apply bool `json:"apply,omitempty"`

	// +kubebuilder:default:=false
	// Suspend control provision jobs
	// If something goes wrong with release we could just update this field
	// And new jobs wouldn't provision
	Suspend bool `json:"suspend,omitempty"`

	// +kubebuilder:default:=false
	// SkipLockUpdatedHosts disables locking mechanism for hosts where salt-formula is applied, so multiple salt-call's
	// can be run in parallel
	SkipLockUpdatedHosts bool `json:"skipLockUpdatedHosts,omitempty"`

	// NodeSelector defines nodes where salt-formula would be applied
	NodeSelector map[string]string `json:"nodeSelector,omitempty"`
}

// SaltFormulaStatus defines the observed state of SaltFormula
type SaltFormulaStatus struct {
	// +kubebuilder:default:=0
	Desired int32 `json:"desired"`
	// +kubebuilder:default:=0
	Current int32 `json:"current"`
	// +kubebuilder:default:=0
	Ready int32 `json:"ready"`

	// Epoch control epoch id for cluster-map
	Epoch string `json:"epoch"`

	// A list of pointers to currently running jobs.
	// +optional
	Active []corev1.ObjectReference `json:"active,omitempty"`

	// Information when was the last time the job was successfully scheduled.
	// +optional
	LastScheduledTime *metav1.Time `json:"lastScheduleTime,omitempty"`
}

// SaltFormula is the Schema for the saltformulas API
//+kubebuilder:object:root=true
//+kubebuilder:subresource:status
//+kubebuilder:printcolumn:name="BaseRole",type=string,JSONPath=`.spec.baseRole`
//+kubebuilder:printcolumn:name="Role",type=string,JSONPath=`.spec.role`
//+kubebuilder:printcolumn:name="Version",type=string,JSONPath=`.spec.version`
//+kubebuilder:printcolumn:name="Desired",type=string,JSONPath=`.status.desired`
//+kubebuilder:printcolumn:name="Current",type=string,JSONPath=`.status.current`
//+kubebuilder:printcolumn:name="Ready",type=string,JSONPath=`.status.ready`
//+kubebuilder:printcolumn:name="Epoch",type=string,JSONPath=`.status.epoch`
type SaltFormula struct {
	metav1.TypeMeta   `json:",inline"`
	metav1.ObjectMeta `json:"metadata,omitempty"`

	Spec   SaltFormulaSpec   `json:"spec,omitempty"`
	Status SaltFormulaStatus `json:"status,omitempty"`
}

//+kubebuilder:object:root=true

// SaltFormulaList contains a list of SaltFormula
type SaltFormulaList struct {
	metav1.TypeMeta `json:",inline"`
	metav1.ListMeta `json:"metadata,omitempty"`
	Items           []SaltFormula `json:"items"`
}

func init() {
	SchemeBuilder.Register(&SaltFormula{}, &SaltFormulaList{})
}
