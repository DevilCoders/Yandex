package dlog

import (
	"fmt"
	"strings"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
)

type LogEntry struct {
	Result steps.RunResult
	Step   steps.DecisionStep
}

type DecisionLog struct {
	entries []LogEntry
}

func (l *DecisionLog) Add(entry LogEntry) {
	l.entries = append(l.entries, entry)
}

func (l *DecisionLog) String() string {
	result := make([]string, len(l.entries))
	for number, e := range l.entries {
		var resLines []string
		prefix := fmt.Sprintf(
			"%2d %s: ",
			number+1, e.Step.GetStepName())
		lines := strings.Split(e.Result.ForHuman, "\n")
		resLines = append(resLines, prefix+lines[0])

		prefix = strings.Repeat(" ", len(prefix))
		if len(lines) > 1 {
			for _, line := range lines[1:] {
				resLines = append(resLines, prefix+line)
			}
		}

		if e.Result.Error != nil {
			resLines = append(resLines, prefix+fmt.Sprintf("an error happened: %s", e.Result.Error))
			resLines = append(resLines, prefix+"I will try again")
		}

		result[number] = strings.Join(resLines, "\n")
	}
	return strings.Join(result, "\n")
}

func (l *DecisionLog) ForCMS() string {
	result := ""
	length := len(l.entries)
	if length > 0 {
		e := l.entries[length-1]
		result = fmt.Sprintf("%s: %s, %s", e.Step.GetStepName(), e.Result.Action, strings.Split(e.Result.ForHuman, "\n")[0])
		if e.Result.Error != nil {
			result += fmt.Sprintf(" error: %s", e.Result.Error)
		}
	}
	return result
}

func NewDecisionLog() DecisionLog {
	return DecisionLog{}
}
