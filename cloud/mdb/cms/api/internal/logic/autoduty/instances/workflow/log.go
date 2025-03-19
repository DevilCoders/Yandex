package workflow

import (
	"fmt"
	"strings"
)

func (wf *BaseWorkflow) StringLog() string {
	result := make([]string, len(wf.stepResults)+len(wf.cleanupStepResults))
	for number, stepResult := range append(wf.stepResults, wf.cleanupStepResults...) {
		var resLines []string
		prefix := fmt.Sprintf(
			"%2d %s: ",
			number+1, stepResult.StepName)
		lines := stepResult.Log
		resLines = append(resLines, prefix+lines[0])

		prefix = strings.Repeat(" ", len(prefix))
		if len(lines) > 1 {
			for _, line := range lines[1:] {
				resLines = append(resLines, prefix+line)
			}
		}

		if stepResult.Error != nil {
			resLines = append(resLines, fmt.Sprintf("%san error happened: %s", prefix, stepResult.Error))
			if stepResult.IsDone {
				resLines = append(resLines, prefix+"Will not retry, this is the final message")
			} else {
				resLines = append(resLines, prefix+"I will try again")
			}
		} else {
			if !stepResult.IsDone {
				resLines = append(resLines, prefix+"Need more time, I will try again later")
			}
			if stepResult.FinishWorkflow {
				resLines = append(resLines, prefix+"Workflow will be finished by this step")
			}
		}

		result[number] = strings.Join(resLines, "\n")
	}
	return strings.Join(result, "\n")
}
