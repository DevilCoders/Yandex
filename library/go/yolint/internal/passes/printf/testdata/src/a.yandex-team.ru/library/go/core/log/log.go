package log

// Logger is the universal logger that can do everything.
type Logger interface {
	loggerFmt
}

type loggerFmt interface {
	// Tracef logs at Trace log level using fmt formatter
	Tracef(format string, args ...interface{})
	// Debugf logs at Debug log level using fmt formatter
	Debugf(format string, args ...interface{})
	// Infof logs at Info log level using fmt formatter
	Infof(format string, args ...interface{})
	// Warnf logs at Warn log level using fmt formatter
	Warnf(format string, args ...interface{})
	// Errorf logs at Error log level using fmt formatter
	Errorf(format string, args ...interface{})
	// Fatalf logs at Fatal log level using fmt formatter
	Fatalf(format string, args ...interface{})
}
