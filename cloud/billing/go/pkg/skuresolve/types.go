package skuresolve

import (
	"github.com/valyala/fastjson"
)

// JMESPath contains jmespath raw formula
type JMESPath string

// MetricValueMatcher is interface for billing metric value checks
type MetricValueMatcher interface {
	MatchValue(interface{}) bool
	MatchJSON(*fastjson.Value) bool
}

// MetricValueMatcher is interface for billing metric values
type MetricValueGetter interface {
	MatchByPath(path string, matcher MetricValueMatcher) (bool, error)
	GetFullValue() *fastjson.Value
}
