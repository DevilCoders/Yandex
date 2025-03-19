package tracetag

import (
	"github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/ext"
)

func RequestID(requestID string) opentracing.Tag {
	return opentracing.Tag{Key: "request_id", Value: requestID}
}

func Source(src string) opentracing.Tag {
	return opentracing.Tag{Key: "source", Value: src}
}

func Handler(name string) opentracing.Tag {
	return opentracing.Tag{Key: "handler", Value: name}
}

func ActionSpan() opentracing.Tag {
	return opentracing.Tag{Key: "span_type", Value: "action"}
}

func Action(value string) opentracing.Tag {
	return opentracing.Tag{Key: "action", Value: value}
}

func IncomingMetricsCount(value int) opentracing.Tag {
	return opentracing.Tag{Key: "incoming_metrics_count", Value: value}
}

func ProcessedMetricsCount(value int) opentracing.Tag {
	return opentracing.Tag{Key: "processed_metrics_count", Value: value}
}

func InvalidMetricsCount(value int) opentracing.Tag {
	return opentracing.Tag{Key: "invald_metrics_count", Value: value}
}

func PushMetricsCount(value int) opentracing.Tag {
	return opentracing.Tag{Key: "push_metrics_count", Value: value}
}

func QuerySpan() opentracing.Tag {
	return opentracing.Tag{Key: "span_type", Value: "query"}
}

func QueryName(value string) opentracing.Tag {
	return opentracing.Tag{Key: "query_name", Value: value}
}

func QueryRowsCount(value int) opentracing.Tag {
	return opentracing.Tag{Key: "query_rows_count", Value: value}
}

func InterconnectSpan() opentracing.Tag {
	return opentracing.Tag{Key: "span_type", Value: "rpc"}
}

func Service(value string) opentracing.Tag {
	return opentracing.Tag{Key: "service", Value: value}
}

func RequestName(value string) opentracing.Tag {
	return opentracing.Tag{Key: "request_name", Value: value}
}

func Failed() opentracing.Tag {
	return opentracing.Tag{Key: "error", Value: true}
}

func RetryAttempt(value int) opentracing.Tag {
	return opentracing.Tag{Key: "retry_attempt", Value: value}
}

func Partition(value int) opentracing.Tag {
	return opentracing.Tag{Key: "partition", Value: value}
}

func Offset(value uint64) opentracing.Tag {
	return opentracing.Tag{Key: "offset", Value: int(value)}
}

func RPCClientSpan() opentracing.Tag {
	return opentracing.Tag{Key: "span.kind", Value: ext.SpanKindRPCClientEnum}
}

func GRPCComponenet() opentracing.Tag {
	return opentracing.Tag{Key: "component", Value: "gRPC"}
}
