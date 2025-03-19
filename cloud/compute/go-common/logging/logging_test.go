package logging

import (
	"testing"
)

func doLogOutput(loggerName string) {
	log := GetLogger(loggerName)
	log.Critical("critical")
	log.Error("error")
	log.Info("info")
	log.Debug("debug")
}

func TestLoggingDebugDevelMode(t *testing.T) {
	MustInitLogging(Config{
		DevelopMode: true,
		DebugMode:   true,
	})

	doLogOutput("debug_develop")
}

func TestLoggingDevelopMode(t *testing.T) {
	MustInitLogging(Config{
		DevelopMode: true,
	})

	doLogOutput("develop")
}

func TestLoggingDebugMode(t *testing.T) {
	MustInitLogging(Config{
		DebugMode: true,
	})

	doLogOutput("debug")
}

func TestLoggingProdMode(t *testing.T) {
	MustInitLogging(Config{
		DebugMode:   false,
		DevelopMode: false,
	})

	doLogOutput("prod")
}
