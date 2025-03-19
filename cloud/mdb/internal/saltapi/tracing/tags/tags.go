package tags

import "a.yandex-team.ru/cloud/mdb/internal/tracing/tags"

var (
	SaltTest       = tags.BoolTagName("salt.test")
	JobFunc        = tags.StringTagName("salt.job.func")
	JobStatesCount = tags.IntTagName("salt.job.states_count")
	StateSLS       = tags.StringTagName("salt.state.sls")
	StateID        = tags.StringTagName("salt.state.id")
	StateStartTime = tags.StringTagName("salt.state.start_time")
	StateDuration  = tags.DurationTagName("salt.state.duration")
	StateResult    = tags.BoolTagName("salt.state.result")
	StateRunNum    = tags.IntTagName("salt.state.run_num")
	RunTarget      = tags.StringTagName("salt.run.target")
)
