package dlog_test

import (
	"context"
	"strings"
	"testing"

	"github.com/stretchr/testify/require"
	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/dlog"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
)

type TestData struct {
	name      string
	in        []dlog.LogEntry
	exp       string
	expForCMS string
}

type TestStep struct {
}

func (s *TestStep) GetStepName() string {
	return "test step"
}

func (s *TestStep) RunStep(_ context.Context, _ *steps.InstructionCtx) steps.RunResult {
	return steps.RunResult{}
}

func happyPaths() []TestData {
	r := []TestData{
		{
			name: "single record",
			in: []dlog.LogEntry{
				{
					Result: steps.RunResult{
						ForHuman: "duty required",
						Action:   steps.AfterStepEscalate,
					},
					Step: &TestStep{},
				},
			},
			exp:       " 1 test step: duty required",
			expForCMS: "test step: escalate, duty required",
		}, {
			name: "error record",
			in: []dlog.LogEntry{
				{
					Result: steps.RunResult{
						ForHuman: "duty required",
						Action:   steps.AfterStepEscalate,
						Error:    xerrors.New("error message"),
					},
					Step: &TestStep{},
				},
			},
			exp: strings.Join([]string{
				" 1 test step: duty required",
				"              an error happened: error message",
				"              I will try again",
			}, "\n"),
			expForCMS: "test step: escalate, duty required error: error message",
		}, {
			name: "two records",
			in: []dlog.LogEntry{
				{
					Result: steps.RunResult{
						ForHuman: "ok",
						Action:   steps.AfterStepContinue,
					},
					Step: &TestStep{},
				}, {
					Result: steps.RunResult{
						ForHuman: "ok",
						Action:   steps.AfterStepContinue,
					},
					Step: &TestStep{},
				},
			},
			exp: strings.Join([]string{
				" 1 test step: ok",
				" 2 test step: ok",
			}, "\n"),
			expForCMS: "test step: continue, ok",
		}, {
			name: "multiline",
			in: []dlog.LogEntry{
				{
					Result: steps.RunResult{
						ForHuman: "line1\nline2",
						Action:   steps.AfterStepContinue,
					},
					Step: &TestStep{},
				},
			},
			exp: strings.Join([]string{
				" 1 test step: line1",
				"              line2",
			}, "\n"),
			expForCMS: "test step: continue, line1",
		},
	}
	return r
}

func TestLogHappyPaths(t *testing.T) {
	for _, tc := range happyPaths() {
		t.Run(tc.name, func(t *testing.T) {
			dl := dlog.DecisionLog{}
			for _, entry := range tc.in {
				dl.Add(entry)
			}
			require.Equal(t, tc.exp, dl.String())
			require.Equal(t, tc.expForCMS, dl.ForCMS())
		})
	}

}
