package logging

import (
	"time"

	"google.golang.org/grpc/codes"

	"a.yandex-team.ru/library/go/core/log"
)

func RequestID(requestID string) log.Field {
	return log.String("request_id", requestID)
}

func OriginRequestID(originRequestID string) log.Field {
	return log.String("origin_request_id", originRequestID)
}

func GRPCCode(code codes.Code) log.Field {
	return log.String("grpc_code", code.String())
}

func DurationMS(duration time.Duration) log.Field {
	return log.Int64("duration_ms", duration.Milliseconds())
}

func HTTPStatusCode(statusCode int) log.Field {
	return log.Int("http_status_code", statusCode)
}

func HTTPUrlPattern(url string) log.Field {
	return log.String("http_url_pattern", url)
}

func HTTPMethod(method string) log.Field {
	return log.String("http_method", method)
}

func ConsoleMessage(message string) log.Field {
	return log.String("console_message", message)
}

func GRPCFullMethod(fullMethod string) log.Field {
	return log.String("grpc_full_method", fullMethod)
}
