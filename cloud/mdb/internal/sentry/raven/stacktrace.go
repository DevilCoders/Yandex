package raven

import (
	"github.com/getsentry/raven-go"

	"a.yandex-team.ru/library/go/core/xerrors"
)

const frameContext = 2

func newStackTrace(err error, skip int, appPackagePrefixes []string) *raven.Stacktrace {
	errStackTrace := xerrors.StackTraceOfCause(err)
	if errStackTrace == nil {
		// No stack traces at all! Collect something and return
		return raven.NewStacktrace(skip+1, frameContext, appPackagePrefixes)
	}

	{
		// Is it a FULL stacktrace?
		frames := errStackTrace.StackTrace().Frames()
		if len(frames) > 1 {
			var res raven.Stacktrace
			for _, fr := range frames {
				stfr := raven.NewStacktraceFrame(fr.PC, fr.Function, fr.File, fr.Line, frameContext, appPackagePrefixes)
				res.Frames = append(res.Frames, stfr)
			}

			// Invert frames (sentry wants 'oldest' first)
			for i, j := 0, len(res.Frames)-1; i < j; i, j = i+1, j-1 {
				res.Frames[i], res.Frames[j] = res.Frames[j], res.Frames[i]
			}

			return &res
		}

		// We are not interested in a single frame here since we need to unwind all frames in correct order
	}

	// Collect current stacktrace
	res := raven.NewStacktrace(skip+1, frameContext, appPackagePrefixes)
	// Apply error frames to it one by one
	errStackTrace = xerrors.StackTraceOfEffect(err)
	for {
		if errStackTrace == nil {
			return res
		}

		frames := errStackTrace.StackTrace().Frames()
		if len(frames) > 0 {
			fr := frames[0]
			frame := raven.NewStacktraceFrame(fr.PC, fr.Function, fr.File, fr.Line, frameContext, appPackagePrefixes)
			// Sentry wants frames oldest first and that is exactly how we extract them
			res.Frames = append(res.Frames, frame)
		}

		errStackTrace = xerrors.NextStackTrace(errStackTrace)
	}
}
