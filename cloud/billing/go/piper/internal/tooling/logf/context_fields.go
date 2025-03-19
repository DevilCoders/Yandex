package logf

import (
	"strings"

	"a.yandex-team.ru/library/go/core/log"
)

func Service(v string) log.Field {
	return log.String("service", v)
}

func SourceName(v string) log.Field {
	return log.String("source_name", v)
}

func Handler(v string) log.Field {
	return log.String("handler", v)
}

func Action(v string) log.Field {
	return log.String("action", v)
}

func RequestID(v string) log.Field {
	return log.String("request_id", v)
}

func QueryName(v string) log.Field {
	return log.String("query_name", v)
}

func QueryRows(v int) log.Field {
	return log.Int("query_rows_count", v)
}

func RequestName(system, name string) log.Field {
	return log.String("request", strings.Join([]string{system, name}, "."))
}

func RetryAttempt(v int) log.Field {
	return log.Int("attempt", v)
}
