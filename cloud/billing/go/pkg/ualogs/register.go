package ualogs

import "go.uber.org/zap"

// RegisterSink for UnifiedAgent with logbroker schema
func RegisterSink() error {
	return zap.RegisterSink("logbroker", NewUnifiedAgentSink)
}
