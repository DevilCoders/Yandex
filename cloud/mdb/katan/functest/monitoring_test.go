package functest

import (
	"context"
	"fmt"
	"regexp"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/monrun"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/cloud/mdb/katan/monitoring/pkg/monitoring"
	"a.yandex-team.ru/library/go/core/log"
)

type MonitoringContext struct {
	kdb    katandb.KatanDB
	L      log.Logger
	result monrun.Result
}

func (mc *MonitoringContext) iExecuteMDBKatanZombieRollouts(ctx context.Context, warn, crit time.Duration) error {
	m, err := monitoring.New(mc.L, mc.kdb, monitoring.DefaultConfig())
	if err != nil {
		return err
	}
	mc.result = m.CheckZombieRollouts(ctx, warn, crit)
	return nil
}

func (mc *MonitoringContext) iExecuteMDBKataBrokenSchedules(ctx context.Context, namespace string) error {
	m, err := monitoring.New(mc.L, mc.kdb, monitoring.DefaultConfig())
	if err != nil {
		return err
	}
	mc.result = m.CheckBrokenSchedules(ctx, namespace, "")
	return nil
}

func (mc *MonitoringContext) lastResultIs(code string) error {
	expectedCode, err := monrun.ResultCodeFromString(code)
	if err != nil {
		return err
	}
	if mc.result.Code != expectedCode {
		return fmt.Errorf("expeceted %q code, but actual is %q: %s", expectedCode, mc.result.Code, mc.result.Message)
	}
	return nil
}

func (mc *MonitoringContext) lastMessageMatches(rs string) error {
	reg, err := regexp.Compile(rs)
	if err != nil {
		return err
	}
	if !reg.MatchString(mc.result.Message) {
		return fmt.Errorf("monitoring message '%s' doesn't match '%s'", mc.result.Message, rs)
	}
	return nil
}
