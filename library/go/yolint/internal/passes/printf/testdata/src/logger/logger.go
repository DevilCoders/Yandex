package logger

import "a.yandex-team.ru/library/go/core/log"

func logErr(l log.Logger) {
	l.Tracef("test: %s") // want `Tracef format %s reads arg #1, but call has 0 args`
	l.Debugf("test: %s") // want `Debugf format %s reads arg #1, but call has 0 args`
	l.Infof("test: %s")  // want `Infof format %s reads arg #1, but call has 0 args`
	l.Warnf("test: %s")  // want `Warnf format %s reads arg #1, but call has 0 args`
	l.Errorf("test: %s") // want `Errorf format %s reads arg #1, but call has 0 args`
	l.Fatalf("test: %s") // want `Fatalf format %s reads arg #1, but call has 0 args`
}
