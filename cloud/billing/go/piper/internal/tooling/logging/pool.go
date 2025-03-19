package logging

import (
	"sync"

	"a.yandex-team.ru/library/go/core/log"
)

func GetFields() *LogFields {
	return logFieldsPool.Get().(*LogFields)
}

func PutFields(f *LogFields) {
	f.Fields = f.Fields[:0]
	logFieldsPool.Put(f)
}

type LogFields struct {
	Fields []log.Field
}

var logFieldsPool = sync.Pool{
	New: func() interface{} {
		return &LogFields{}
	},
}
