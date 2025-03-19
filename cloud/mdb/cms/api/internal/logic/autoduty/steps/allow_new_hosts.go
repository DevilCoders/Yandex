package steps

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/settings"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
)

type AllowNewHostsStep struct {
	dbm   dbm.Client
	value bool
}

func (s *AllowNewHostsStep) GetStepName() string {
	return "allow_new_hosts"
}

func (s *AllowNewHostsStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	info, err := s.dbm.AreNewHostsAllowed(ctx, rd.R.MustOneFQDN())
	if err != nil {
		return waitWithErrAndMessage(err, "could not get list of containers on dom0")
	}
	if info.NewHostsAllowed == s.value {
		if info.SetBy != settings.CMSRobotLogin {
			return continueWithMessage(
				fmt.Sprintf("allow_new_hosts==%v already, but set by '%s'. Anyway, I continue",
					s.value, info.SetBy))
		}
	} else {
		if s.value && info.SetBy != settings.CMSRobotLogin {
			return continueWithMessage(
				fmt.Sprintf("left allow_new_hosts==false, because it was explicitly set not by me, but by '%s'. New hosts will not be placed on this DOM0",
					info.SetBy))
		}
		err = s.dbm.UpdateNewHostsAllowed(ctx, rd.R.MustOneFQDN(), s.value)
		if err != nil {
			return waitWithErrAndMessage(err, "could not set flag 'allow_new_hosts' in DBM")
		}
	}
	return continueWithMessage(fmt.Sprintf("set to %v", s.value))
}

func NewAllowNewHostsStep(dbm dbm.Client, flagValue bool) DecisionStep {
	return &AllowNewHostsStep{dbm: dbm, value: flagValue}
}
