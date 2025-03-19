package supp

import (
	"a.yandex-team.ru/library/go/core/log"
)

// DoAction collect all changable actions in code into one func
// save action to change list as msg, log it as msg and do action if no dry mode as act() func
func DoAction(l log.Logger, dryRun bool, changes []string, msg string, act func() error) ([]string, error) {
	return append(changes, msg), DoActionWithoutChangelist(l, dryRun, msg, act)
}

// DoActionWithoutChangelist collect all changable actions in code into one func
// log it as msg and do action if no dry mode as act() func
func DoActionWithoutChangelist(l log.Logger, dryRun bool, msg string, act func() error) error {
	if dryRun {
		l.Infof("dry run %s", msg)
	} else {
		l.Info(msg)
		err := act()
		if err != nil {
			return err
		}
		l.Debugf("done %s", msg)
	}
	return nil
}
