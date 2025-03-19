package logf

import "a.yandex-team.ru/library/go/core/log"

func Offset(v uint64) log.Field {
	return log.UInt64("offset", v)
}

func Size(v int) log.Field {
	return log.Int("size", v)
}

func Error(err error) log.Field {
	return log.Error(err)
}

func ErrorKey(v string) log.Field {
	return log.String("error_key", v)
}

func ErrorValue(v string) log.Field {
	return log.String("error_value", v)
}

func Partition(v int) log.Field {
	return log.Int("partition", v)
}

func Value(v string) log.Field {
	return log.String("value", v)
}
