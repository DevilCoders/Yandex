package xiva

import "encoding/json"

type Filter struct {
	Rules []interface{}          `json:"rules"`
	Vars  map[string]interface{} `json:"vars"` // temporary workaround https://st.yandex-team.ru/RTEC-4170
}

type Action string

const (
	SendBright Action = "send_bright"
	SendSilent Action = "send_silent"
	Skip              = "skip"
)

type ConditionalRule struct {
	Condition interface{} `json:"if"`
	Do        Action      `json:"do"`
}

type ActionRule struct {
	Do Action `json:"do"`
}

func NewFilter() *Filter {
	return &Filter{Vars: map[string]interface{}{}}
}

func (f *Filter) AppendKeyEquals(key string, values []string, action Action) *Filter {
	rule := ConditionalRule{
		Do: action,
		Condition: map[string]interface{}{
			key: map[string][]string{"$eq": values},
		},
	}
	f.Rules = append(f.Rules, rule)
	return f
}

func (f *Filter) AppendEvent(values []string, action Action) *Filter {
	rule := ConditionalRule{
		Do: action,
		Condition: map[string]interface{}{
			"$event": values,
		},
	}
	f.Rules = append(f.Rules, rule)
	return f
}

func (f *Filter) AppendAction(action Action) *Filter {
	rule := ActionRule{Do: action}
	f.Rules = append(f.Rules, rule)
	return f
}

func (f *Filter) String() (string, error) {
	if len(f.Rules) == 0 {
		return "", nil
	}

	encoded, err := json.Marshal(f)
	if err != nil {
		return "", err
	}

	return string(encoded), nil
}
