package panictools

import (
	"context"
	"fmt"
	"runtime"
	"strings"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	buildinfo "a.yandex-team.ru/mds/go/xbuildinfo"
)

// LogRecovery is function to log panic.
func LogRecovery(ctx context.Context, logger log.Logger, p interface{}) {
	err, ok := p.(error)
	if !ok {
		err = fmt.Errorf("%v", p)
	}

	panicCall := GetPanicCall()
	fields := []log.Field{
		log.Error(err),
	}

	if panicCall.Function != "" {
		fields = append(
			fields,
			log.String("panic_func", panicCall.Function),
			log.Sprintf(
				"panic_file", "%s:%d",
				strings.TrimPrefix(panicCall.File, buildinfo.GetBuildInfo().OtherInfo.TopSrcDir), panicCall.Line,
			),
		)
	}

	ctxlog.Error(ctx, logger, "internal error caused by panic", fields...)
}

// GetPanicCall tries to find where panic was occurred
func GetPanicCall() (frame runtime.Frame) {
	pc := make([]uintptr, 16)
	n := runtime.Callers(1, pc)
	frames := runtime.CallersFrames(pc[:n])

	var more bool

	// skip until panic call
	for {
		frame, more = frames.Next()
		if !more || frame.Function == "runtime.gopanic" {
			break
		}
	}

	if !more {
		return
	}

	// skip runtime package funcs
	for {
		frame, more = frames.Next()
		if !more || !strings.HasPrefix(frame.Function, "runtime.") {
			break
		}
	}

	return
}
