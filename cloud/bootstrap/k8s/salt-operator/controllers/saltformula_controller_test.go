package controllers

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"reflect"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"
	kbatch "k8s.io/api/batch/v1"
	corev1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	kruntime "k8s.io/apimachinery/pkg/runtime"
	"k8s.io/apimachinery/pkg/types"
	"k8s.io/client-go/kubernetes/scheme"
	controllerruntime "sigs.k8s.io/controller-runtime"
	"sigs.k8s.io/controller-runtime/pkg/client"
	"sigs.k8s.io/controller-runtime/pkg/client/fake"
	"sigs.k8s.io/controller-runtime/pkg/reconcile"

	bootstrapv1alpha1 "a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/api/v1alpha1"
	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/internal/locks"
	mocklocks "a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/internal/locks/mock"
	ydblock "a.yandex-team.ru/cloud/bootstrap/ydb-lock"
)

func TestSaltFormulaReconciler_Reconcile(t *testing.T) {
	// Register operator types with the runtime scheme.
	s := scheme.Scheme
	err := bootstrapv1alpha1.AddToScheme(s)
	if err != nil {
		t.Error(err)
	}

	c := &config{
		bootstrapHostEndpoint: bootstrapHost,
		saltRunnerClient:      saltRunnerClient,
		saltRunnerSocketDir:   saltRunnerSocketDir,
		saltRunnerSocketName:  saltRunnerSocketName,
	}

	type fields struct {
		Client  client.Client
		Scheme  *kruntime.Scheme
		config  *config
		ydblock locks.Locker
	}
	type args struct {
		req      controllerruntime.Request
		loadFunc func(c client.Client)
	}
	tests := []struct {
		name      string
		fields    fields
		args      args
		want      controllerruntime.Result
		wantErr   bool
		wantFunc  func(c client.Client) error
		setupFunc func(r *SaltFormulaReconciler, ctrl *gomock.Controller)
	}{
		{
			name: "Test max fail not set",
			fields: fields{
				Client: fake.NewClientBuilder().WithObjects().Build(),
				Scheme: s,
				config: c,
			},
			args: args{
				req: newRequest("bar", "foo"),
				loadFunc: func(c client.Client) {
					_ = c.Create(context.TODO(), &bootstrapv1alpha1.SaltFormula{
						ObjectMeta: metav1.ObjectMeta{
							Name:      "foo",
							Namespace: "bar",
						},
						Spec: bootstrapv1alpha1.SaltFormulaSpec{
							BaseRole:             "test-role",
							Version:              "0.1-9999",
							BatchSize:            1,
							SkipLockUpdatedHosts: true,
							NodeSelector: map[string]string{
								"label1": "value",
							},
						},
					})
					for i := 0; i < 10; i++ {
						_ = c.Create(context.TODO(), &corev1.Node{
							ObjectMeta: metav1.ObjectMeta{
								Name: fmt.Sprintf("node%d", i),
								Labels: map[string]string{
									"label1": "value",
								},
							},
							Status: corev1.NodeStatus{
								Conditions: []corev1.NodeCondition{
									{Type: corev1.NodeReady, Status: corev1.ConditionTrue},
								},
							},
						})
					}
					for i := 0; i < 5; i++ {
						controller := true
						_ = c.Create(context.TODO(), &kbatch.Job{
							ObjectMeta: metav1.ObjectMeta{
								Namespace: "bar",
								Name:      fmt.Sprintf("job%d", i),
								OwnerReferences: []metav1.OwnerReference{
									{
										Name:       "foo",
										Kind:       "SaltFormula",
										Controller: &controller,
									},
								},
								Labels: map[string]string{
									"salt_formula_version": "0.1-9999",
								},
							},
							Spec: kbatch.JobSpec{
								Template: corev1.PodTemplateSpec{
									Spec: corev1.PodSpec{
										NodeSelector: map[string]string{
											"kubernetes.io/hostname": fmt.Sprintf("node%d", i),
										},
									},
								},
							},
							Status: kbatch.JobStatus{
								Conditions: []kbatch.JobCondition{
									{Type: kbatch.JobFailed, Status: corev1.ConditionTrue},
								},
							},
						})
					}
				},
			},
			want: controllerruntime.Result{RequeueAfter: requeueAfter},
			wantFunc: func(c client.Client) error {
				jobList := &kbatch.JobList{}
				listOptions := []client.ListOption{
					client.InNamespace("bar"),
				}
				if err := c.List(context.TODO(), jobList, listOptions...); err != nil {
					return err
				}
				if len(jobList.Items) != 6 {
					return fmt.Errorf("len(jobList.Items) is not equal to 6, current: %d", len(jobList.Items))
				}

				return nil
			},
			setupFunc: func(r *SaltFormulaReconciler, ctrl *gomock.Controller) {
				mock := mocklocks.NewMockLocker(ctrl)
				r.ydblock = mock
			},
		},
		{
			name: "Test max fail set",
			fields: fields{
				Client: fake.NewClientBuilder().WithObjects().Build(),
				Scheme: s,
				config: c,
			},
			args: args{
				req: newRequest("bar", "foo"),
				loadFunc: func(c client.Client) {
					_ = c.Create(context.TODO(), &bootstrapv1alpha1.SaltFormula{
						ObjectMeta: metav1.ObjectMeta{
							Name:      "foo",
							Namespace: "bar",
						},
						Spec: bootstrapv1alpha1.SaltFormulaSpec{
							BaseRole:  "test-role",
							Version:   "0.1-9999",
							MaxFail:   5,
							BatchSize: 1,
							NodeSelector: map[string]string{
								"label1": "value",
							},
						},
					})
					for i := 0; i < 10; i++ {
						_ = c.Create(context.TODO(), &corev1.Node{
							ObjectMeta: metav1.ObjectMeta{
								Name: fmt.Sprintf("node%d", i),
								Labels: map[string]string{
									"label1": "value",
								},
							},
							Status: corev1.NodeStatus{
								Conditions: []corev1.NodeCondition{
									{Type: corev1.NodeReady, Status: corev1.ConditionTrue},
								},
							},
						})
					}
					for i := 0; i < 5; i++ {
						controller := true
						_ = c.Create(context.TODO(), &kbatch.Job{
							ObjectMeta: metav1.ObjectMeta{
								Namespace: "bar",
								Name:      fmt.Sprintf("job%d", i),
								OwnerReferences: []metav1.OwnerReference{
									{
										Name:       "foo",
										Kind:       "SaltFormula",
										Controller: &controller,
									},
								},
								Labels: map[string]string{
									"salt_formula_version": "0.1-9999",
								},
							},
							Spec: kbatch.JobSpec{
								Template: corev1.PodTemplateSpec{
									Spec: corev1.PodSpec{
										NodeSelector: map[string]string{
											"kubernetes.io/hostname": fmt.Sprintf("node%d", i),
										},
									},
								},
							},
							Status: kbatch.JobStatus{
								Conditions: []kbatch.JobCondition{
									{Type: kbatch.JobFailed, Status: corev1.ConditionTrue},
								},
							},
						})
					}
				},
			},
			want: controllerruntime.Result{},
			wantFunc: func(c client.Client) error {
				jobList := &kbatch.JobList{}
				listOptions := []client.ListOption{
					client.InNamespace("bar"),
				}
				if err := c.List(context.TODO(), jobList, listOptions...); err != nil {
					return err
				}
				if len(jobList.Items) != 5 {
					return fmt.Errorf("len(jobList.Items) is not equal to 5, current: %d", len(jobList.Items))
				}

				return nil
			},
		},
		{
			name: "Test create lock",
			fields: fields{
				Client: fake.NewClientBuilder().WithObjects().Build(),
				Scheme: s,
				config: c,
			},
			args: args{
				req: newRequest("bar", "foo"),
				loadFunc: func(c client.Client) {
					_ = c.Create(context.TODO(), &bootstrapv1alpha1.SaltFormula{
						ObjectMeta: metav1.ObjectMeta{
							Name:      "foo",
							Namespace: "bar",
						},
						Spec: bootstrapv1alpha1.SaltFormulaSpec{
							BaseRole:  "test-role",
							Version:   "0.1-9999",
							BatchSize: 2,
							NodeSelector: map[string]string{
								"label1": "value",
							},
						},
					})
					for i := 0; i < 2; i++ {
						_ = c.Create(context.TODO(), &corev1.Node{
							ObjectMeta: metav1.ObjectMeta{
								Name: fmt.Sprintf("node%d", i),
								Labels: map[string]string{
									"label1": "value",
								},
							},
							Status: corev1.NodeStatus{
								Conditions: []corev1.NodeCondition{
									{Type: corev1.NodeReady, Status: corev1.ConditionTrue},
								},
							},
						})
					}
				},
			},
			want: controllerruntime.Result{RequeueAfter: requeueAfter},
			wantFunc: func(c client.Client) error {
				jobList := &kbatch.JobList{}
				listOptions := []client.ListOption{
					client.InNamespace("bar"),
				}
				if err := c.List(context.TODO(), jobList, listOptions...); err != nil {
					return err
				}
				if len(jobList.Items) != 2 {
					return fmt.Errorf("len(jobList.Items) is not equal to 2, current: %d", len(jobList.Items))
				}
				for _, job := range jobList.Items {
					jsonLock, ok := job.Annotations[lockAnnotation]
					assert.True(t, ok)
					var lock ydblock.Lock
					err = json.Unmarshal([]byte(jsonLock), &lock)
					assert.NoError(t, err)
					assert.Equal(t, lock.Hosts, []string{job.Spec.Template.Spec.NodeSelector[hostnameLabel]})
					assert.Equal(t, lock.Deadline, uint64(0))
					assert.Contains(t, lock.Description, "Updating via salt-operator with filter <salt_roles= roles=test-role k8s_labels={\"label1\":\"value\"}> (jobID=")
					assert.Equal(t, lock.Timeout, uint64(0))
					assert.Equal(t, lock.Type, ydblock.HostLockTypeOther)
				}

				return nil
			},
			setupFunc: func(r *SaltFormulaReconciler, ctrl *gomock.Controller) {
				mock := mocklocks.NewMockLocker(ctrl)
				mock.EXPECT().CreateLock(
					gomock.Any(),
					gomock.Any(),
					gomock.Any(),
					gomock.Eq(uint64(0)),
					gomock.Eq(ydblock.HostLockTypeOther),
				).Times(2).DoAndReturn(
					func(
						ctx context.Context,
						description string,
						hosts []string,
						hbTimeout uint64,
						lockType ydblock.HostLockType,
					) (*ydblock.Lock, error) {
						return &ydblock.Lock{
							Hosts:       hosts,
							Deadline:    0,
							Description: description,
							Timeout:     hbTimeout,
							Type:        lockType,
						}, nil
					})
				r.ydblock = mock
			},
		},
		{
			name: "Test create lock skipped",
			fields: fields{
				Client: fake.NewClientBuilder().WithObjects().Build(),
				Scheme: s,
				config: c,
			},
			args: args{
				req: newRequest("bar", "foo"),
				loadFunc: func(c client.Client) {
					_ = c.Create(context.TODO(), &bootstrapv1alpha1.SaltFormula{
						ObjectMeta: metav1.ObjectMeta{
							Name:      "foo",
							Namespace: "bar",
						},
						Spec: bootstrapv1alpha1.SaltFormulaSpec{
							BaseRole:             "test-role",
							Version:              "0.1-9999",
							BatchSize:            2,
							SkipLockUpdatedHosts: true,
							NodeSelector: map[string]string{
								"label1": "value",
							},
						},
					})
					for i := 0; i < 2; i++ {
						_ = c.Create(context.TODO(), &corev1.Node{
							ObjectMeta: metav1.ObjectMeta{
								Name: fmt.Sprintf("node%d", i),
								Labels: map[string]string{
									"label1": "value",
								},
							},
							Status: corev1.NodeStatus{
								Conditions: []corev1.NodeCondition{
									{Type: corev1.NodeReady, Status: corev1.ConditionTrue},
								},
							},
						})
					}
				},
			},
			want: controllerruntime.Result{RequeueAfter: requeueAfter},
			wantFunc: func(c client.Client) error {
				jobList := &kbatch.JobList{}
				listOptions := []client.ListOption{
					client.InNamespace("bar"),
				}
				if err := c.List(context.TODO(), jobList, listOptions...); err != nil {
					return err
				}
				if len(jobList.Items) != 2 {
					return fmt.Errorf("len(jobList.Items) is not equal to 2, current: %d", len(jobList.Items))
				}
				for _, job := range jobList.Items {
					_, ok := job.Annotations[lockAnnotation]
					assert.False(t, ok)
				}

				return nil
			},
			setupFunc: func(r *SaltFormulaReconciler, ctrl *gomock.Controller) {
				mock := mocklocks.NewMockLocker(ctrl)
				r.ydblock = mock
			},
		},
		{
			name: "Test extend lock",
			fields: fields{
				Client: fake.NewClientBuilder().WithObjects().Build(),
				Scheme: s,
				config: c,
			},
			args: args{
				req: newRequest("bar", "foo"),
				loadFunc: func(c client.Client) {
					_ = c.Create(context.TODO(), &bootstrapv1alpha1.SaltFormula{
						ObjectMeta: metav1.ObjectMeta{
							Name:      "foo",
							Namespace: "bar",
						},
						Spec: bootstrapv1alpha1.SaltFormulaSpec{
							BaseRole:  "test-role",
							Version:   "0.1-9999",
							BatchSize: 2,
							NodeSelector: map[string]string{
								"label1": "value",
							},
						},
					})
					for i := 0; i < 2; i++ {
						_ = c.Create(context.TODO(), &corev1.Node{
							ObjectMeta: metav1.ObjectMeta{
								Name: fmt.Sprintf("node%d", i),
								Labels: map[string]string{
									"label1": "value",
								},
							},
							Status: corev1.NodeStatus{
								Conditions: []corev1.NodeCondition{
									{Type: corev1.NodeReady, Status: corev1.ConditionTrue},
								},
							},
						})
					}
					for i := 0; i < 2; i++ {
						jsonLock, _ := json.Marshal(ydblock.Lock{})
						controller := true
						_ = c.Create(context.TODO(), &kbatch.Job{
							ObjectMeta: metav1.ObjectMeta{
								Namespace: "bar",
								Name:      fmt.Sprintf("job%d", i),
								OwnerReferences: []metav1.OwnerReference{
									{
										Name:       "foo",
										Kind:       "SaltFormula",
										Controller: &controller,
									},
								},
								Labels: map[string]string{
									"salt_formula_version": "0.1-9999",
								},
								Annotations: map[string]string{
									lockAnnotation: string(jsonLock),
								},
							},
							Spec: kbatch.JobSpec{
								Template: corev1.PodTemplateSpec{
									Spec: corev1.PodSpec{
										NodeSelector: map[string]string{
											"kubernetes.io/hostname": fmt.Sprintf("node%d", i),
										},
									},
								},
							},
						})
					}
				},
			},
			want: controllerruntime.Result{RequeueAfter: requeueAfter},
			wantFunc: func(c client.Client) error {
				jobList := &kbatch.JobList{}
				listOptions := []client.ListOption{
					client.InNamespace("bar"),
				}
				if err := c.List(context.TODO(), jobList, listOptions...); err != nil {
					return err
				}
				if len(jobList.Items) != 2 {
					return fmt.Errorf("len(jobList.Items) is not equal to 2, current: %d", len(jobList.Items))
				}
				for _, job := range jobList.Items {
					jsonLock, ok := job.Annotations[lockAnnotation]
					assert.True(t, ok)
					var lock ydblock.Lock
					err = json.Unmarshal([]byte(jsonLock), &lock)
					assert.NoError(t, err)
					assert.Equal(t, lock.Deadline, uint64(100))
				}

				return nil
			},
			setupFunc: func(r *SaltFormulaReconciler, ctrl *gomock.Controller) {
				mock := mocklocks.NewMockLocker(ctrl)
				mock.EXPECT().ExtendLock(
					gomock.Any(),
					gomock.Any(),
				).Times(2).DoAndReturn(
					func(
						ctx context.Context,
						lock *ydblock.Lock,
					) (*ydblock.Lock, error) {
						lock.Deadline = 100
						return lock, nil
					})
				r.ydblock = mock
			},
		},
		{
			name: "Test extend lock skipped",
			fields: fields{
				Client: fake.NewClientBuilder().WithObjects().Build(),
				Scheme: s,
				config: c,
			},
			args: args{
				req: newRequest("bar", "foo"),
				loadFunc: func(c client.Client) {
					_ = c.Create(context.TODO(), &bootstrapv1alpha1.SaltFormula{
						ObjectMeta: metav1.ObjectMeta{
							Name:      "foo",
							Namespace: "bar",
						},
						Spec: bootstrapv1alpha1.SaltFormulaSpec{
							BaseRole:             "test-role",
							Version:              "0.1-9999",
							BatchSize:            2,
							SkipLockUpdatedHosts: true,
							NodeSelector: map[string]string{
								"label1": "value",
							},
						},
					})
					for i := 0; i < 2; i++ {
						_ = c.Create(context.TODO(), &corev1.Node{
							ObjectMeta: metav1.ObjectMeta{
								Name: fmt.Sprintf("node%d", i),
								Labels: map[string]string{
									"label1": "value",
								},
							},
							Status: corev1.NodeStatus{
								Conditions: []corev1.NodeCondition{
									{Type: corev1.NodeReady, Status: corev1.ConditionTrue},
								},
							},
						})
					}
					for i := 0; i < 2; i++ {
						jsonLock, _ := json.Marshal(ydblock.Lock{})
						controller := true
						_ = c.Create(context.TODO(), &kbatch.Job{
							ObjectMeta: metav1.ObjectMeta{
								Namespace: "bar",
								Name:      fmt.Sprintf("job%d", i),
								OwnerReferences: []metav1.OwnerReference{
									{
										Name:       "foo",
										Kind:       "SaltFormula",
										Controller: &controller,
									},
								},
								Labels: map[string]string{
									"salt_formula_version": "0.1-9999",
								},
								Annotations: map[string]string{
									lockAnnotation: string(jsonLock),
								},
							},
							Spec: kbatch.JobSpec{
								Template: corev1.PodTemplateSpec{
									Spec: corev1.PodSpec{
										NodeSelector: map[string]string{
											"kubernetes.io/hostname": fmt.Sprintf("node%d", i),
										},
									},
								},
							},
						})
					}
				},
			},
			want: controllerruntime.Result{RequeueAfter: requeueAfter},
			wantFunc: func(c client.Client) error {
				jobList := &kbatch.JobList{}
				listOptions := []client.ListOption{
					client.InNamespace("bar"),
				}
				if err := c.List(context.TODO(), jobList, listOptions...); err != nil {
					return err
				}
				if len(jobList.Items) != 2 {
					return fmt.Errorf("len(jobList.Items) is not equal to 2, current: %d", len(jobList.Items))
				}

				return nil
			},
			setupFunc: func(r *SaltFormulaReconciler, ctrl *gomock.Controller) {
				mock := mocklocks.NewMockLocker(ctrl)
				r.ydblock = mock
			},
		},
		{
			name: "Test missing lock annotation",
			fields: fields{
				Client: fake.NewClientBuilder().WithObjects().Build(),
				Scheme: s,
				config: c,
			},
			args: args{
				req: newRequest("bar", "foo"),
				loadFunc: func(c client.Client) {
					_ = c.Create(context.TODO(), &bootstrapv1alpha1.SaltFormula{
						ObjectMeta: metav1.ObjectMeta{
							Name:      "foo",
							Namespace: "bar",
						},
						Spec: bootstrapv1alpha1.SaltFormulaSpec{
							BaseRole:  "test-role",
							Version:   "0.1-9999",
							BatchSize: 2,
							NodeSelector: map[string]string{
								"label1": "value",
							},
						},
					})
					for i := 0; i < 2; i++ {
						_ = c.Create(context.TODO(), &corev1.Node{
							ObjectMeta: metav1.ObjectMeta{
								Name: fmt.Sprintf("node%d", i),
								Labels: map[string]string{
									"label1": "value",
								},
							},
							Status: corev1.NodeStatus{
								Conditions: []corev1.NodeCondition{
									{Type: corev1.NodeReady, Status: corev1.ConditionTrue},
								},
							},
						})
					}
					for i := 0; i < 2; i++ {
						controller := true
						_ = c.Create(context.TODO(), &kbatch.Job{
							ObjectMeta: metav1.ObjectMeta{
								Namespace: "bar",
								Name:      fmt.Sprintf("job%d", i),
								OwnerReferences: []metav1.OwnerReference{
									{
										Name:       "foo",
										Kind:       "SaltFormula",
										Controller: &controller,
									},
								},
								Labels: map[string]string{
									"salt_formula_version": "0.1-9999",
								},
							},
							Spec: kbatch.JobSpec{
								Template: corev1.PodTemplateSpec{
									Spec: corev1.PodSpec{
										NodeSelector: map[string]string{
											"kubernetes.io/hostname": fmt.Sprintf("node%d", i),
										},
									},
								},
							},
						})
					}
				},
			},
			want: controllerruntime.Result{RequeueAfter: requeueAfter},
			wantFunc: func(c client.Client) error {
				jobList := &kbatch.JobList{}
				listOptions := []client.ListOption{
					client.InNamespace("bar"),
				}
				if err := c.List(context.TODO(), jobList, listOptions...); err != nil {
					return err
				}
				if len(jobList.Items) != 2 {
					return fmt.Errorf("len(jobList.Items) is not equal to 2, current: %d", len(jobList.Items))
				}
				for _, job := range jobList.Items {
					jsonLock, ok := job.Annotations[lockAnnotation]
					assert.True(t, ok)
					var lock ydblock.Lock
					err = json.Unmarshal([]byte(jsonLock), &lock)
					assert.NoError(t, err)
				}

				return nil
			},
			setupFunc: func(r *SaltFormulaReconciler, ctrl *gomock.Controller) {
				mock := mocklocks.NewMockLocker(ctrl)
				mock.EXPECT().CreateLock(
					gomock.Any(),
					gomock.Any(),
					gomock.Any(),
					gomock.Eq(uint64(0)),
					gomock.Eq(ydblock.HostLockTypeOther),
				).Times(2).DoAndReturn(
					func(
						ctx context.Context,
						description string,
						hosts []string,
						hbTimeout uint64,
						lockType ydblock.HostLockType,
					) (*ydblock.Lock, error) {
						return &ydblock.Lock{
							Hosts:       hosts,
							Deadline:    0,
							Description: description,
							Timeout:     hbTimeout,
							Type:        lockType,
						}, nil
					})
				r.ydblock = mock
			},
		},
		{
			name: "Test unmarshal error",
			fields: fields{
				Client: fake.NewClientBuilder().WithObjects().Build(),
				Scheme: s,
				config: c,
			},
			args: args{
				req: newRequest("bar", "foo"),
				loadFunc: func(c client.Client) {
					_ = c.Create(context.TODO(), &bootstrapv1alpha1.SaltFormula{
						ObjectMeta: metav1.ObjectMeta{
							Name:      "foo",
							Namespace: "bar",
						},
						Spec: bootstrapv1alpha1.SaltFormulaSpec{
							BaseRole:  "test-role",
							Version:   "0.1-9999",
							BatchSize: 2,
							NodeSelector: map[string]string{
								"label1": "value",
							},
						},
					})
					for i := 0; i < 2; i++ {
						_ = c.Create(context.TODO(), &corev1.Node{
							ObjectMeta: metav1.ObjectMeta{
								Name: fmt.Sprintf("node%d", i),
								Labels: map[string]string{
									"label1": "value",
								},
							},
							Status: corev1.NodeStatus{
								Conditions: []corev1.NodeCondition{
									{Type: corev1.NodeReady, Status: corev1.ConditionTrue},
								},
							},
						})
					}
					for i := 0; i < 2; i++ {
						controller := true
						_ = c.Create(context.TODO(), &kbatch.Job{
							ObjectMeta: metav1.ObjectMeta{
								Namespace: "bar",
								Name:      fmt.Sprintf("job%d", i),
								OwnerReferences: []metav1.OwnerReference{
									{
										Name:       "foo",
										Kind:       "SaltFormula",
										Controller: &controller,
									},
								},
								Labels: map[string]string{
									"salt_formula_version": "0.1-9999",
								},
								Annotations: map[string]string{
									lockAnnotation: "",
								},
							},
							Spec: kbatch.JobSpec{
								Template: corev1.PodTemplateSpec{
									Spec: corev1.PodSpec{
										NodeSelector: map[string]string{
											"kubernetes.io/hostname": fmt.Sprintf("node%d", i),
										},
									},
								},
							},
						})
					}
				},
			},
			want: controllerruntime.Result{RequeueAfter: requeueAfter},
			wantFunc: func(c client.Client) error {
				jobList := &kbatch.JobList{}
				listOptions := []client.ListOption{
					client.InNamespace("bar"),
				}
				if err := c.List(context.TODO(), jobList, listOptions...); err != nil {
					return err
				}
				if len(jobList.Items) != 2 {
					return fmt.Errorf("len(jobList.Items) is not equal to 2, current: %d", len(jobList.Items))
				}
				for _, job := range jobList.Items {
					jsonLock, ok := job.Annotations[lockAnnotation]
					assert.True(t, ok)
					var lock ydblock.Lock
					err = json.Unmarshal([]byte(jsonLock), &lock)
					assert.NoError(t, err)
				}

				return nil
			},
			setupFunc: func(r *SaltFormulaReconciler, ctrl *gomock.Controller) {
				mock := mocklocks.NewMockLocker(ctrl)
				mock.EXPECT().CreateLock(
					gomock.Any(),
					gomock.Any(),
					gomock.Any(),
					gomock.Eq(uint64(0)),
					gomock.Eq(ydblock.HostLockTypeOther),
				).Times(2).DoAndReturn(
					func(
						ctx context.Context,
						description string,
						hosts []string,
						hbTimeout uint64,
						lockType ydblock.HostLockType,
					) (*ydblock.Lock, error) {
						return &ydblock.Lock{
							Hosts:       hosts,
							Deadline:    0,
							Description: description,
							Timeout:     hbTimeout,
							Type:        lockType,
						}, nil
					})
				r.ydblock = mock
			},
		},
		{
			name: "Test release lock",
			fields: fields{
				Client: fake.NewClientBuilder().WithObjects().Build(),
				Scheme: s,
				config: c,
			},
			args: args{
				req: newRequest("bar", "foo"),
				loadFunc: func(c client.Client) {
					_ = c.Create(context.TODO(), &bootstrapv1alpha1.SaltFormula{
						ObjectMeta: metav1.ObjectMeta{
							Name:      "foo",
							Namespace: "bar",
						},
						Spec: bootstrapv1alpha1.SaltFormulaSpec{
							BaseRole:  "test-role",
							Version:   "0.1-9999",
							BatchSize: 2,
							NodeSelector: map[string]string{
								"label1": "value",
							},
						},
					})
					for i := 0; i < 2; i++ {
						_ = c.Create(context.TODO(), &corev1.Node{
							ObjectMeta: metav1.ObjectMeta{
								Name: fmt.Sprintf("node%d", i),
								Labels: map[string]string{
									"label1": "value",
								},
							},
							Status: corev1.NodeStatus{
								Conditions: []corev1.NodeCondition{
									{Type: corev1.NodeReady, Status: corev1.ConditionTrue},
								},
							},
						})
					}
					for i := 0; i < 2; i++ {
						jsonLock, _ := json.Marshal(ydblock.Lock{})
						controller := true
						_ = c.Create(context.TODO(), &kbatch.Job{
							ObjectMeta: metav1.ObjectMeta{
								Namespace: "bar",
								Name:      fmt.Sprintf("job%d", i),
								OwnerReferences: []metav1.OwnerReference{
									{
										Name:       "foo",
										Kind:       "SaltFormula",
										Controller: &controller,
									},
								},
								Labels: map[string]string{
									"salt_formula_version": "0.1-9999",
								},
								Annotations: map[string]string{
									lockAnnotation: string(jsonLock),
								},
							},
							Spec: kbatch.JobSpec{
								Template: corev1.PodTemplateSpec{
									Spec: corev1.PodSpec{
										NodeSelector: map[string]string{
											"kubernetes.io/hostname": fmt.Sprintf("node%d", i),
										},
									},
								},
							},
							Status: kbatch.JobStatus{
								Conditions: []kbatch.JobCondition{
									{Type: kbatch.JobComplete, Status: corev1.ConditionTrue},
								},
							},
						})
					}
				},
			},
			want: controllerruntime.Result{},
			setupFunc: func(r *SaltFormulaReconciler, ctrl *gomock.Controller) {
				mock := mocklocks.NewMockLocker(ctrl)
				mock.EXPECT().ReleaseLock(
					gomock.Any(),
					gomock.Any(),
				).Times(2).DoAndReturn(
					func(
						ctx context.Context,
						lock *ydblock.Lock,
					) error {
						return nil
					})
				r.ydblock = mock
			},
		},
		{
			name: "Test release lock error",
			fields: fields{
				Client: fake.NewClientBuilder().WithObjects().Build(),
				Scheme: s,
				config: c,
			},
			args: args{
				req: newRequest("bar", "foo"),
				loadFunc: func(c client.Client) {
					_ = c.Create(context.TODO(), &bootstrapv1alpha1.SaltFormula{
						ObjectMeta: metav1.ObjectMeta{
							Name:      "foo",
							Namespace: "bar",
						},
						Spec: bootstrapv1alpha1.SaltFormulaSpec{
							BaseRole:  "test-role",
							Version:   "0.1-9999",
							BatchSize: 2,
							NodeSelector: map[string]string{
								"label1": "value",
							},
						},
					})
					for i := 0; i < 2; i++ {
						_ = c.Create(context.TODO(), &corev1.Node{
							ObjectMeta: metav1.ObjectMeta{
								Name: fmt.Sprintf("node%d", i),
								Labels: map[string]string{
									"label1": "value",
								},
							},
							Status: corev1.NodeStatus{
								Conditions: []corev1.NodeCondition{
									{Type: corev1.NodeReady, Status: corev1.ConditionTrue},
								},
							},
						})
					}
					for i := 0; i < 2; i++ {
						jsonLock, _ := json.Marshal(ydblock.Lock{})
						controller := true
						_ = c.Create(context.TODO(), &kbatch.Job{
							ObjectMeta: metav1.ObjectMeta{
								Namespace: "bar",
								Name:      fmt.Sprintf("job%d", i),
								OwnerReferences: []metav1.OwnerReference{
									{
										Name:       "foo",
										Kind:       "SaltFormula",
										Controller: &controller,
									},
								},
								Labels: map[string]string{
									"salt_formula_version": "0.1-9999",
								},
								Annotations: map[string]string{
									lockAnnotation: string(jsonLock),
								},
							},
							Spec: kbatch.JobSpec{
								Template: corev1.PodTemplateSpec{
									Spec: corev1.PodSpec{
										NodeSelector: map[string]string{
											"kubernetes.io/hostname": fmt.Sprintf("node%d", i),
										},
									},
								},
							},
							Status: kbatch.JobStatus{
								Conditions: []kbatch.JobCondition{
									{Type: kbatch.JobComplete, Status: corev1.ConditionTrue},
								},
							},
						})
					}
				},
			},
			want: controllerruntime.Result{},
			setupFunc: func(r *SaltFormulaReconciler, ctrl *gomock.Controller) {
				mock := mocklocks.NewMockLocker(ctrl)
				mock.EXPECT().ReleaseLock(
					gomock.Any(),
					gomock.Any(),
				).Times(2).DoAndReturn(
					func(
						ctx context.Context,
						lock *ydblock.Lock,
					) error {
						return errors.New("")
					})
				r.ydblock = mock
			},
		},
		{
			name: "Test node selector",
			fields: fields{
				Client: fake.NewClientBuilder().WithObjects().Build(),
				Scheme: s,
				config: c,
			},
			args: args{
				req: newRequest("bar", "foo"),
				loadFunc: func(c client.Client) {
					_ = c.Create(context.TODO(), &bootstrapv1alpha1.SaltFormula{
						ObjectMeta: metav1.ObjectMeta{
							Name:      "foo",
							Namespace: "bar",
						},
						Spec: bootstrapv1alpha1.SaltFormulaSpec{
							Version:   "0.1-9999",
							BaseRole:  "test-role",
							BatchSize: 3,
							NodeSelector: map[string]string{
								"label1": "value",
							},
							SkipLockUpdatedHosts: true,
						},
					})
					for i := 0; i < 2; i++ {
						_ = c.Create(context.TODO(), &corev1.Node{
							ObjectMeta: metav1.ObjectMeta{
								Name: fmt.Sprintf("node%d", i),
								Labels: map[string]string{
									"label1": "value",
								},
							},
							Status: corev1.NodeStatus{
								Conditions: []corev1.NodeCondition{
									{Type: corev1.NodeReady, Status: corev1.ConditionTrue},
								},
							},
						})
					}
					_ = c.Create(context.TODO(), &corev1.Node{
						ObjectMeta: metav1.ObjectMeta{
							Name: "node2",
						},
						Status: corev1.NodeStatus{
							Conditions: []corev1.NodeCondition{
								{Type: corev1.NodeReady, Status: corev1.ConditionTrue},
							},
						},
					})
				},
			},
			want: controllerruntime.Result{RequeueAfter: requeueAfter},
			wantFunc: func(c client.Client) error {
				jobList := &kbatch.JobList{}
				listOptions := []client.ListOption{
					client.InNamespace("bar"),
				}
				if err := c.List(context.TODO(), jobList, listOptions...); err != nil {
					return err
				}
				assert.Equal(t, 2, len(jobList.Items))

				return nil
			},
		},
		{
			name: "Test nil node selector",
			fields: fields{
				Client: fake.NewClientBuilder().WithObjects().Build(),
				Scheme: s,
				config: c,
			},
			args: args{
				req: newRequest("bar", "foo"),
				loadFunc: func(c client.Client) {
					_ = c.Create(context.TODO(), &bootstrapv1alpha1.SaltFormula{
						ObjectMeta: metav1.ObjectMeta{
							Name:      "foo",
							Namespace: "bar",
						},
						Spec: bootstrapv1alpha1.SaltFormulaSpec{
							Version:              "0.1-9999",
							BaseRole:             "test-role",
							BatchSize:            3,
							SkipLockUpdatedHosts: true,
						},
					})
					for i := 0; i < 2; i++ {
						_ = c.Create(context.TODO(), &corev1.Node{
							ObjectMeta: metav1.ObjectMeta{
								Name: fmt.Sprintf("node%d", i),
								Labels: map[string]string{
									"label1": "value",
								},
							},
							Status: corev1.NodeStatus{
								Conditions: []corev1.NodeCondition{
									{Type: corev1.NodeReady, Status: corev1.ConditionTrue},
								},
							},
						})
					}
					_ = c.Create(context.TODO(), &corev1.Node{
						ObjectMeta: metav1.ObjectMeta{
							Name: "node2",
						},
						Status: corev1.NodeStatus{
							Conditions: []corev1.NodeCondition{
								{Type: corev1.NodeReady, Status: corev1.ConditionTrue},
							},
						},
					})
				},
			},
			wantErr: true,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()

			r := &SaltFormulaReconciler{
				Client: tt.fields.Client,
				Scheme: tt.fields.Scheme,
				config: tt.fields.config,
			}
			if tt.args.loadFunc != nil {
				tt.args.loadFunc(r.Client)
			}
			if tt.setupFunc != nil {
				tt.setupFunc(r, ctrl)
			}
			got, err := r.Reconcile(context.TODO(), tt.args.req)
			if (err != nil) != tt.wantErr {
				t.Errorf("Reconcile() error = %v, wantErr %v", err, tt.wantErr)
				return
			}
			if !reflect.DeepEqual(got, tt.want) {
				t.Errorf("Reconcile() got = %v, want %v", got, tt.want)
			}
			if tt.wantFunc != nil {
				if err := tt.wantFunc(r.Client); err != nil {
					t.Errorf("Reconcile() wantFunc validation error: %v", err)
				}
			}
		})
	}
}

func newRequest(ns, name string) reconcile.Request {
	return reconcile.Request{
		NamespacedName: types.NamespacedName{
			Namespace: ns,
			Name:      name,
		},
	}
}
