package internal

import (
	"context"
	"encoding/json"
	"io/ioutil"
	"reflect"
	"strconv"
	"time"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type TelegrafMetricField struct {
	Value     interface{} `json:"value"`
	UpdatedAt time.Time   `json:"updated_at"`
}

var intType = reflect.TypeOf(int64(1))
var boolType = reflect.TypeOf(true)

var stringType = reflect.TypeOf("")

func getInt(value interface{}) (int64, error) {
	v := reflect.ValueOf(value)
	if v.Type().ConvertibleTo(intType) {
		iv := v.Convert(intType)
		return iv.Int(), nil
	} else if v.Type().ConvertibleTo(stringType) {
		sv := v.Convert(stringType)
		return strconv.ParseInt(sv.String(), 10, 64)
	} else if v.Type().ConvertibleTo(boolType) {
		bv := v.Convert(boolType)
		if bv.Bool() {
			return 1, nil
		} else {
			return 0, nil
		}
	}
	return 0, xerrors.Errorf("Can't convert %v to int64", v.Type())
}

func (app *App) GetStatus(ctx context.Context, now time.Time) (bool, bool, error) {
	data, err := ioutil.ReadFile(app.Cfg.TelegrafStateFile)
	if err != nil {
		return false, false, xerrors.Errorf("failed to read telegraf state file: %w", err)
	}
	metrics := make(map[string]TelegrafMetricField)
	err = json.Unmarshal(data, &metrics)
	if err != nil {
		return false, false, err
	}
	isAlive := true
	for name, val := range app.Cfg.TelegrafChecks {
		aliveMetric, ok := metrics[name]
		if !ok || now.Sub(aliveMetric.UpdatedAt) > StateTTL {
			return false, false, xerrors.Errorf("%s metric is stale or absent", name)
		}
		v1, err := getInt(val)
		if err != nil {
			return false, false, xerrors.Errorf("invalid telegraf_check metric %s value %v: (can't convert to int)", name, val)
		}
		v2, err := getInt(aliveMetric.Value)
		if err != nil {
			return false, false, xerrors.Errorf("invalid telegraf metric %s value %v: (can't convert to int)", name, aliveMetric.Value)
		}
		if v1 != v2 {
			app.L().Warnf("telegraf check %s has failed %v != %v", name, v1, v2)
			isAlive = false
		}
	}
	needSendLicense := app.Cfg.SendLicenseMetrics && isAlive && len(app.Cfg.TelegrafLicenseChecks) > 0
	for name, val := range app.Cfg.TelegrafLicenseChecks {
		primaryMetric, ok := metrics[name]
		if !ok || now.Sub(primaryMetric.UpdatedAt) > StateTTL {
			return false, false, xerrors.Errorf("%s metric is stale or absent", name)
		}
		v1, err := getInt(val)
		if err != nil {
			return false, false, xerrors.Errorf("invalid telegraf_license_check metric %s value %v: (can't convert to int)", name, val)
		}
		v2, err := getInt(primaryMetric.Value)
		if err != nil {
			return false, false, xerrors.Errorf("invalid telegraf metric %s value %v: (can't convert to int)", name, primaryMetric.Value)
		}
		if v1 != v2 {
			app.L().Debugf("telegraf primary check %s failed %v != %v", name, v1, v2)
			needSendLicense = false
		}
	}
	return isAlive, needSendLicense, nil
}
