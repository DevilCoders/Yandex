package workflow

import (
	"testing"

	"github.com/stretchr/testify/require"
	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
)

func TestBaseWorkflow_LetterAfterCleanupInOKPending(t *testing.T) {
	type fields struct {
		cleanupResults []steps.RunResult
	}
	tests := []struct {
		name   string
		fields fields
		want   Letter
	}{
		{
			name: "reject if cleanup error",
			fields: fields{
				cleanupResults: []steps.RunResult{
					{
						Error:  xerrors.New("test-error"),
						IsDone: true,
					},
				},
			},
			want: Reject,
		},
		{
			name: "done if no error",
			fields: fields{
				cleanupResults: []steps.RunResult{
					{
						Error:  nil,
						IsDone: true,
					},
				},
			},
			want: OK,
		},
		{
			name: "undone if not done result",
			fields: fields{
				cleanupResults: []steps.RunResult{
					{
						Error:  nil,
						IsDone: false,
					},
				},
			},
			want: OKPending,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			wf := &BaseWorkflow{
				cleanupStepResults: tt.fields.cleanupResults,
			}
			require.Equal(t, tt.want, wf.LetterAfterCleanupInOKPending())
		})
	}
}

func TestBaseWorkflow_LetterAfterCleanupInRejectPending(t *testing.T) {
	type fields struct {
		cleanupResults []steps.RunResult
	}
	tests := []struct {
		name   string
		fields fields
		want   Letter
	}{
		{
			name: "reject if done with error",
			fields: fields{
				cleanupResults: []steps.RunResult{
					{
						Error:  xerrors.New("test-error"),
						IsDone: true,
					},
				},
			},
			want: Reject,
		},
		{
			name: "reject if done without error",
			fields: fields{
				cleanupResults: []steps.RunResult{
					{
						Error:  nil,
						IsDone: true,
					},
				},
			},
			want: Reject,
		},
		{
			name: "pending rejected if not done",
			fields: fields{
				cleanupResults: []steps.RunResult{
					{
						IsDone: false,
					},
				},
			},
			want: RejectPending,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			wf := &BaseWorkflow{
				cleanupStepResults: tt.fields.cleanupResults,
			}
			require.Equal(t, tt.want, wf.LetterAfterCleanupInRejectPending())
		})
	}
}

func TestBaseWorkflow_LetterAfterProcessingInProgress(t *testing.T) {
	type fields struct {
		stepResults []steps.RunResult
	}
	tests := []struct {
		name   string
		fields fields
		want   Letter
	}{
		{
			name: "if not yet done, leave in progress",
			fields: fields{
				stepResults: []steps.RunResult{
					{
						IsDone: false,
					},
				},
			},
			want: InProgress,
		},
		{
			name: "done with error start rejecting",
			fields: fields{
				stepResults: []steps.RunResult{
					{
						Error:  xerrors.New("test-error"),
						IsDone: true,
					},
				},
			},
			want: RejectPending,
		},
		{
			name: "done with no error start OKking",
			fields: fields{
				stepResults: []steps.RunResult{
					{
						Error:  nil,
						IsDone: true,
					},
				},
			},
			want: OKPending,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			wf := &BaseWorkflow{
				stepResults: tt.fields.stepResults,
			}
			require.Equal(t, tt.want, wf.LetterAfterProcessingInProgress())
		})
	}
}

func TestBaseWorkflow_LastResult(t *testing.T) {
	type fields struct {
		stepResults        []steps.RunResult
		cleanupStepResults []steps.RunResult
	}
	tests := []struct {
		name   string
		fields fields
		want   steps.RunResult
	}{
		{
			name: "cleanup result",
			fields: fields{
				cleanupStepResults: []steps.RunResult{
					{
						Description: "cleanup",
					},
				},
			},
			want: steps.RunResult{
				Description: "cleanup",
			},
		},
		{
			name: "usual run result",
			fields: fields{
				stepResults: []steps.RunResult{
					{
						Description: "usual",
					},
				},
			},
			want: steps.RunResult{
				Description: "usual",
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			wf := &BaseWorkflow{
				stepResults:        tt.fields.stepResults,
				cleanupStepResults: tt.fields.cleanupStepResults,
			}
			require.Equal(t, tt.want, wf.LastResult())
		})
	}
}
