package saltapi

import (
	"encoding/json"
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
)

func TestReturnResult_IsSuccessful_ParseLevel(t *testing.T) {
	for _, level := range []ParseLevel{ParseLevelNone, ParseLevelMinimal} {
		t.Run(string(level), func(t *testing.T) {
			assert.Panics(t, func() { ReturnResult{ParseLevel: level}.IsSuccessful() })
		})
	}

	for _, level := range []ParseLevel{ParseLevelPartial, ParseLevelFull} {
		t.Run(string(level), func(t *testing.T) {
			assert.NotPanics(t, func() { ReturnResult{ParseLevel: level}.IsSuccessful() })
		})
	}
}

func TestReturnResult_IsSuccessful(t *testing.T) {
	fakeReturnStates := map[string]*StateReturn{"foo": {}}
	data := []struct {
		name         string
		input        ReturnResult
		isSuccessful bool
	}{
		{
			name: "success ok states full",
			input: ReturnResult{
				Success:      true,
				ReturnCode:   returnCodeOK,
				ReturnStates: fakeReturnStates,
				ParseLevel:   ParseLevelFull,
			},
			isSuccessful: true,
		},
		{
			name: "success ok slsfunc full",
			input: ReturnResult{
				Success:    true,
				ReturnCode: returnCodeOK,
				Func:       stateSLSFunc,
				ParseLevel: ParseLevelFull,
			},
			isSuccessful: true,
		},
		{
			name: "ok states full",
			input: ReturnResult{
				ReturnCode:   returnCodeOK,
				ReturnStates: fakeReturnStates,
				ParseLevel:   ParseLevelFull,
			},
		},
		{
			name: "ok slsfunc full",
			input: ReturnResult{
				ReturnCode: returnCodeOK,
				Func:       stateSLSFunc,
				ParseLevel: ParseLevelFull,
			},
		},
		{
			name: "success resulterror states full",
			input: ReturnResult{
				Success:      true,
				ReturnCode:   returnCodeResultError,
				ReturnStates: fakeReturnStates,
				ParseLevel:   ParseLevelFull,
			},
		},
		{
			name: "success resulterror slsfunc full",
			input: ReturnResult{
				Success:    true,
				ReturnCode: returnCodeResultError,
				Func:       stateSLSFunc,
				ParseLevel: ParseLevelFull,
			},
			isSuccessful: true,
		},
		{
			name: "success resulterror slsfunc states full",
			input: ReturnResult{
				Success:      true,
				ReturnCode:   returnCodeResultError,
				Func:         stateSLSFunc,
				ReturnStates: fakeReturnStates,
				ParseLevel:   ParseLevelFull,
			},
		},
		{
			name: "success resulterror full",
			input: ReturnResult{
				Success:    true,
				ReturnCode: returnCodeResultError,
				ParseLevel: ParseLevelFull,
			},
		},
		{
			name: "success compilererror full",
			input: ReturnResult{
				Success:    true,
				ReturnCode: returnCodeCompilerError,
				ParseLevel: ParseLevelFull,
			},
		},
		{
			name: "success compilererror slsfunc full",
			input: ReturnResult{
				Success:    true,
				ReturnCode: returnCodeCompilerError,
				Func:       stateSLSFunc,
				ParseLevel: ParseLevelFull,
			},
		},
		{
			name: "success pillarcompilererror full",
			input: ReturnResult{
				Success:    true,
				ReturnCode: returnCodePillarCompileError,
				ParseLevel: ParseLevelFull,
			},
		},
		{
			name: "success pillarcompilererror slsfunc full",
			input: ReturnResult{
				Success:    true,
				ReturnCode: returnCodePillarCompileError,
				Func:       stateSLSFunc,
				ParseLevel: ParseLevelFull,
			},
		},
		{
			name: "success code42 full",
			input: ReturnResult{
				Success:    true,
				ReturnCode: 42,
				ParseLevel: ParseLevelFull,
			},
		},
		{
			name: "success code42 slsfunc full",
			input: ReturnResult{
				Success:    true,
				ReturnCode: 42,
				Func:       stateSLSFunc,
				ParseLevel: ParseLevelFull,
			},
		},
		{
			name: "success code2 nilstate full",
			input: ReturnResult{
				Success:      true,
				ReturnCode:   2,
				Func:         stateSLSFunc,
				ReturnStates: map[string]*StateReturn{"cmd_|-run-data-move-script_|-/usr/local/yandex/data_move.sh front_|-run": nil},
				ParseLevel:   ParseLevelFull,
			},
		},
	}

	for _, d := range data {
		t.Run(fmt.Sprintf("ReturnResult"+d.name), func(t *testing.T) {
			assert.Equal(t, d.isSuccessful, d.input.IsSuccessful())
		})
	}
}

func TestParseJID(t *testing.T) {
	inputs := []struct {
		Name string
		JID  string
		Time time.Time
		Err  bool
	}{
		{
			Name: "empty",
			Err:  true,
		},
		{
			Name: "too small",
			JID:  "1",
			Err:  true,
		},
		{
			Name: "too big",
			JID:  "012345678901234567890123456789",
			Err:  true,
		},
		{
			Name: "symbols",
			JID:  "aaaaaaaaaaaaaaaaaaaa",
			Err:  true,
		},
		{
			Name: "valid",
			JID:  "20120328010203005000",
			Time: time.Date(2012, 3, 28, 1, 2, 3, 5000000, time.UTC),
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			ts, err := parseJID(input.JID)
			if input.Err {
				require.Error(t, err)
				return
			}

			require.NoError(t, err)
			assert.Equal(t, input.Time, ts)
		})
	}
}

func TestDateFromTime(t *testing.T) {
	inputs := []struct {
		Time time.Time
		Date time.Time
	}{
		{
			Time: time.Date(2020, 5, 21, 0, 0, 0, 0, time.UTC),
			Date: time.Date(2020, 5, 21, 0, 0, 0, 0, time.UTC),
		},
		{
			Time: time.Date(2020, 5, 21, 12, 0, 0, 0, time.UTC),
			Date: time.Date(2020, 5, 21, 0, 0, 0, 0, time.UTC),
		},
		{
			Time: time.Date(2020, 5, 21, 23, 59, 59, 0, time.UTC),
			Date: time.Date(2020, 5, 21, 0, 0, 0, 0, time.UTC),
		},
		{
			Time: time.Date(2020, 5, 31, 18, 30, 35, 39308000, time.UTC),
			Date: time.Date(2020, 5, 31, 0, 0, 0, 0, time.UTC),
		},
	}

	for _, input := range inputs {
		t.Run(input.Time.String(), func(t *testing.T) {
			require.Equal(t, input.Date, dateFromTime(input.Time))
		})
	}
}

func TestParseStartTimePoint(t *testing.T) {
	inputs := []struct {
		Name            string
		BaseTimePoint   time.Duration
		Point           string
		Multiplier      time.Duration
		ResultTimePoint time.Duration
		Err             bool
	}{
		{
			Name: "empty",
			Err:  true,
		},
		{
			Name:  "not a number",
			Point: "a",
			Err:   true,
		},
		{
			Name:            "hour",
			Point:           "11",
			Multiplier:      time.Hour,
			ResultTimePoint: 11 * time.Hour,
		},
		{
			Name:            "minute",
			Point:           "11",
			Multiplier:      time.Minute,
			ResultTimePoint: 11 * time.Minute,
		},
		{
			Name:            "minute",
			Point:           "11",
			Multiplier:      time.Second,
			ResultTimePoint: 11 * time.Second,
		},
		{
			Name:            "microsecond",
			Point:           "005000",
			Multiplier:      time.Microsecond,
			ResultTimePoint: 5 * time.Millisecond,
		},
		{
			Name:            "addition",
			BaseTimePoint:   time.Hour + 10*time.Minute + 20*time.Second,
			Point:           "11",
			Multiplier:      time.Second,
			ResultTimePoint: time.Hour + 10*time.Minute + 20*time.Second + 11*time.Second,
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			timePoint, err := parseStartTimePoint(input.BaseTimePoint, input.Point, input.Multiplier)
			if input.Err {
				require.Error(t, err)
				return
			}

			require.NoError(t, err)
			require.Equal(t, input.ResultTimePoint, timePoint)
		})
	}
}

func TestParseStartTime(t *testing.T) {
	inputs := []struct {
		Name      string
		StartDate time.Time
		StartTime string
		TZOffset  time.Duration
		Res       time.Time
		Err       bool
	}{
		{
			Name: "empty",
			Err:  true,
		},
		{
			Name:      "symbol",
			StartTime: "a",
			Err:       true,
		},
		{
			Name:      "valid",
			StartDate: time.Date(2012, 3, 28, 0, 0, 0, 0, time.UTC),
			StartTime: "13:56:19.018018",
			Res:       time.Date(2012, 3, 28, 13, 56, 19, 18018000, time.UTC),
		},
		{
			Name:      "tz offset",
			StartDate: time.Date(2012, 3, 28, 0, 0, 0, 0, time.UTC),
			StartTime: "13:56:19.018018",
			TZOffset:  time.Hour * 3,
			Res:       time.Date(2012, 3, 28, 10, 56, 19, 18018000, time.UTC),
		},
		{
			Name:      "no microseconds",
			StartDate: time.Date(2012, 3, 28, 0, 0, 0, 0, time.UTC),
			StartTime: "13:56:19",
			Res:       time.Date(2012, 3, 28, 13, 56, 19, 0, time.UTC),
		},
		{
			Name:      "MDB-8427",
			StartDate: time.Date(2020, 5, 21, 22, 49, 27, 131255000, time.UTC),
			StartTime: "01:49:32.999548",
			TZOffset:  time.Hour * 3,
			Res:       time.Date(2020, 5, 21, 22, 49, 32, 999548000, time.UTC),
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			res, err := parseStartTime(input.StartDate, input.StartTime, input.TZOffset)
			if input.Err {
				require.Error(t, err)
				return
			}

			require.NoError(t, err)
			require.Equal(t, input.Res, res)
		})
	}
}

const (
	mdb8425ReturnResult = `
{
	"id": "rc1c-9fspm73yzt6ophv2.mdb.yandexcloud.net",
	"fun": "state.sls",
	"jid": "20200521224927131255",
	"return": {
		"cmd_|-run-data-move-script_|-/usr/local/yandex/data_move.sh front_|-run": {
			"name": "/usr/local/yandex/data_move.sh front",
			"__id__": "run-data-move-script",
			"result": false,
			"__sls__": "components.dbaas-operations.run-data-move-front-script",
			"changes": {},
			"comment": "Command \"/usr/local/yandex/data_move.sh front\" run",
			"duration": 4071.046,
			"start_time": "01:49:32.999548",
			"__run_num__": 0
		}
	},
	"retcode": 2,
	"success": true,
	"fun_args": [
		"components.dbaas-operations.run-data-move-front-script",
		"pillar={'feature_flags': ['MDB_MONGODB_40', 'MDB_LOCAL_DISK_RESIZE', 'MDB_POSTGRESQL_10_1C', 'MDB_MYSQL', 'MDB_MYSQL_8_0', 'MDB_POSTGRESQL_12', 'MDB_HADOOP_ALPHA', 'GA', 'MDB_POSTGRESQL_11', 'EGRESS_NAT_ALPHA', 'MDB_MONGODB_42_RS_UPGRADE', 'MDB_HADOOP_GPU', 'MDB_MONGODB_4_2_SHARDED_UPGRADE']}",
		"concurrent=True",
		"saltenv=compute-prod",
		"timeout=2048000"
	]
}
`
	cmdretcodeReturnResult = `
{
	"id": "sas-05hjtuae942xrxcw.test.net2367",
	"fun": "cmd.retcode",
	"jid": "20200525130613185353",
	"return": 0,
	"retcode": 0,
	"success": true,
	"fun_args": [
		"echo 'ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAG5qz2rEv+/DWWGOe76Yr8i4UPMfeKB7sib3PJ3fZ/csDZSmCq/7Q+PnihxHP0EvSxvhYDNtVdM+Z04VqC0lIPMtAD77CCjZ3U4yS80tR/x0uJjBm4LxGPrYh7vYJA4JBGAjJRuWB2whQTYVxPQVCBVx7is/SGoJrjCzMainHCPaa4DgA== robot-pgaas-deploy' >> /root/.ssh/authorized_keys2",
		"python_shell=True",
		"timeout=60"
	]
}
`
)

func TestParseReturnResult(t *testing.T) {
	inputs := []struct {
		Name       string
		Data       []byte
		TZOffset   time.Duration
		Result     ReturnResult
		Err        bool
		Successful bool
	}{
		{
			Name: "empty",
			Err:  true,
		},
		{
			Name:     "MDB-8427",
			Data:     []byte(mdb8425ReturnResult),
			TZOffset: 3 * time.Hour,
			Result: ReturnResult{
				FQDN: "rc1c-9fspm73yzt6ophv2.mdb.yandexcloud.net",
				Func: "state.sls",
				FuncArgs: []string{
					"components.dbaas-operations.run-data-move-front-script",
					"pillar={'feature_flags': ['MDB_MONGODB_40', 'MDB_LOCAL_DISK_RESIZE', 'MDB_POSTGRESQL_10_1C', 'MDB_MYSQL', 'MDB_MYSQL_8_0', 'MDB_POSTGRESQL_12', 'MDB_HADOOP_ALPHA', 'GA', 'MDB_POSTGRESQL_11', 'EGRESS_NAT_ALPHA', 'MDB_MONGODB_42_RS_UPGRADE', 'MDB_HADOOP_GPU', 'MDB_MONGODB_4_2_SHARDED_UPGRADE']}",
					"concurrent=True",
					"saltenv=compute-prod",
					"timeout=2048000",
				},
				JID:      "20200521224927131255",
				StartTS:  time.Date(2020, 5, 21, 22, 49, 27, 131255000, time.UTC),
				FinishTS: time.Date(2020, 5, 21, 22, 49, 37, 70594000, time.UTC),
				ReturnStates: map[string]*StateReturn{
					"cmd_|-run-data-move-script_|-/usr/local/yandex/data_move.sh front_|-run": {
						Name:      "/usr/local/yandex/data_move.sh front",
						ID:        "run-data-move-script",
						Result:    false,
						SLS:       "components.dbaas-operations.run-data-move-front-script",
						Changes:   json.RawMessage(`{}`),
						Comment:   "Command \"/usr/local/yandex/data_move.sh front\" run",
						Duration:  encodingutil.Duration{Duration: 4*time.Second + 71046*time.Microsecond},
						StartTS:   time.Date(2020, 5, 21, 22, 49, 32, 999548000, time.UTC),
						FinishTS:  time.Date(2020, 5, 21, 22, 49, 37, 70594000, time.UTC),
						StartTime: "01:49:32.999548",
						RunNum:    0,
					},
				},
				ReturnCode: 2,
				Success:    true,
				ParseLevel: ParseLevelFull,
			},
		},
		{
			Name:     "cmd.retcode",
			Data:     []byte(cmdretcodeReturnResult),
			TZOffset: 3 * time.Hour,
			Result: ReturnResult{
				FQDN: "sas-05hjtuae942xrxcw.test.net2367",
				Func: "cmd.retcode",
				FuncArgs: []string{
					"echo 'ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAG5qz2rEv+/DWWGOe76Yr8i4UPMfeKB7sib3PJ3fZ/csDZSmCq/7Q+PnihxHP0EvSxvhYDNtVdM+Z04VqC0lIPMtAD77CCjZ3U4yS80tR/x0uJjBm4LxGPrYh7vYJA4JBGAjJRuWB2whQTYVxPQVCBVx7is/SGoJrjCzMainHCPaa4DgA== robot-pgaas-deploy' >> /root/.ssh/authorized_keys2",
					"python_shell=True",
					"timeout=60",
				},
				JID:         "20200525130613185353",
				StartTS:     time.Date(2020, 5, 25, 13, 6, 13, 185353000, time.UTC),
				FinishTS:    time.Date(2020, 5, 25, 13, 6, 13, 185353000, time.UTC),
				ReturnValue: float64(0),
				ReturnCode:  0,
				Success:     true,
				ParseLevel:  ParseLevelFull,
			},
			Successful: true,
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			rr, errs := ParseReturnResult(input.Data, input.TZOffset)
			if input.Err {
				require.NotEmpty(t, errs)
				return
			}

			require.Empty(t, errs)
			assertEqualReturnResult(t, input.Result, rr)
			assert.Equal(t, input.Successful, rr.IsSuccessful())
			assert.NoError(t, rr.validate())
		})
	}
}

func assertEqualReturnResult(t *testing.T, expected, actual ReturnResult) {
	assert.Equal(t, expected.FQDN, actual.FQDN)
	assert.Equal(t, expected.Func, actual.Func)
	assert.Equal(t, expected.FuncArgs, actual.FuncArgs)
	assert.Equal(t, expected.JID, actual.JID)
	assert.Equal(t, expected.StartTS, actual.StartTS)
	assert.Equal(t, expected.FinishTS, actual.FinishTS)
	assert.Equal(t, expected.ReturnStates, actual.ReturnStates)
	assert.Equal(t, expected.ReturnValue, actual.ReturnValue)
	// We ignore ReturnRaw
	assert.Equal(t, expected.ReturnCode, actual.ReturnCode)
	assert.Equal(t, expected.Success, actual.Success)
	assert.Equal(t, expected.ParseLevel, actual.ParseLevel)
}
