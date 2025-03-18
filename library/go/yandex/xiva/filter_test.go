package xiva

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func Test_RulesBuilding(t *testing.T) {
	var inputs = []struct {
		Name           string
		TestFilter     *Filter
		ExpectedResult string
	}{
		{
			Name: "Build simple key filter",
			TestFilter: NewFilter().
				AppendKeyEquals("device_id", []string{"123321"}, SendBright).
				AppendAction(Skip),
			ExpectedResult: `{"rules":[{"if":{"device_id":{"$eq":["123321"]}},"do":"send_bright"},{"do":"skip"}],"vars":{}}`,
		},
		{
			Name: "Build simple event filter",
			TestFilter: NewFilter().
				AppendEvent([]string{"big bada boom"}, SendBright).
				AppendAction(Skip),
			ExpectedResult: `{"rules":[{"if":{"$event":["big bada boom"]},"do":"send_bright"},{"do":"skip"}],"vars":{}}`,
		},
		{
			Name: "Build complex filter",
			TestFilter: NewFilter().
				AppendKeyEquals("device_id", []string{"123"}, SendBright).
				AppendKeyEquals("channel_id", []string{"456", "789"}, SendSilent).
				AppendEvent([]string{"chicken good", "leeloo dallas multipass"}, SendSilent).
				AppendAction(Skip),
			ExpectedResult: `{"rules":[{"if":{"device_id":{"$eq":["123"]}},"do":"send_bright"},{"if":{"channel_id":{"$eq":["456","789"]}},"do":"send_silent"},{"if":{"$event":["chicken good","leeloo dallas multipass"]},"do":"send_silent"},{"do":"skip"}],"vars":{}}`,
		},
		{
			Name:           "Build empty rules chain",
			TestFilter:     NewFilter(),
			ExpectedResult: "",
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			rules, err := input.TestFilter.String()
			if err != nil {
				t.Errorf("failed to build the rule: %v", err)
			}
			assert.Equal(t, input.ExpectedResult, rules)
		})
	}
}
