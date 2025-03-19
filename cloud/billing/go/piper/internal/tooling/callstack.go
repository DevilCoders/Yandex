package tooling

import (
	"runtime"
	"strings"
)

type callstack struct {
	pc [3]uintptr
}

func getCallFuncName(skip int) string {
	skipFrames := skip + 1

	var pc [2]uintptr
	if runtime.Callers(skipFrames, pc[:]) == 0 {
		return ""
	}

	frames := runtime.CallersFrames(pc[:])
	if _, ok := frames.Next(); !ok {
		return ""
	}
	fr, _ := frames.Next()

	// NOTE: in frame we have a.yandex-team.ru/full/path/to/packet.(optionalReceiverType).CallerFunction
	// and want only CallerFunction
	i := strings.LastIndex(fr.Function, ".")
	return fr.Function[i+1:]
}

func getPanicCall() (frame runtime.Frame) {
	pc := make([]uintptr, 16)
	n := runtime.Callers(1, pc)
	frames := runtime.CallersFrames(pc[:n])
	more := false
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
