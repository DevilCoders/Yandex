package logf

import (
	"google.golang.org/grpc/codes"

	"a.yandex-team.ru/library/go/core/log"
)

func HTTPStatus(v int) log.Field {
	return log.Int("status", v)
}

func HTTPBody(v []byte) log.Field {
	l := len(v)
	if l > 1024 {
		l = 1024
	}
	return log.String("body", string(v[:l]))
}

func GRPCMethod(v string) log.Field {
	return log.String("grpc_method", v)
}

func GRPCCode(v codes.Code) log.Field {
	return log.Int("status", int(v))
}

func LBTopic(v string) log.Field {
	return log.String("topic", v)
}
