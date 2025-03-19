package saltapi

import (
	"encoding/json"
	"fmt"
	"strconv"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

// ReturnResult is the result of one job
type ReturnResult struct {
	FQDN     string   `json:"fqdn" yaml:"fqdn"`
	Func     string   `json:"func" yaml:"func"`
	FuncArgs []string `json:"func_args" yaml:"func_args"`
	JID      string   `json:"jid" yaml:"jid"`

	// StartTS is calculated from JID
	StartTS time.Time `json:"start_ts" yaml:"start_ts"`
	// FinishTS is calculated from JID and return states if any are present
	FinishTS time.Time `json:"finish_ts" yaml:"finish_ts"`

	// ReturnStates shows up when we executed something like 'state.highstate', with multiple states
	ReturnStates ReturnStatesMap `json:"return_states,omitempty" yaml:"return_states,omitempty"`
	// ReturnValue shows up when we executed something simple like 'cmd.run' or 'cmd.retcode'
	ReturnValue interface{} `json:"return_value,omitempty" yaml:"return_value,omitempty"`
	// ReturnRaw is always present and holds raw return value
	ReturnRaw json.RawMessage `json:"-" yaml:"-"`

	// ReturnCode indicates if command execution was successful.
	ReturnCode int `json:"return_code" yaml:"return_code"`
	// Success indicates that command was executed. It does NOT indicate that command was successful.
	Success bool `json:"success" yaml:"success"`

	// ParseLevel indicates how much of the result was actually parsed
	ParseLevel ParseLevel `json:"-" yaml:"-"`
}

func (rr ReturnResult) validate() error {
	if rr.StartTS.After(rr.FinishTS) {
		return xerrors.Errorf("rr.StartTS.After(rr.FinishTS): %s vs %s", rr.StartTS, rr.FinishTS)
	}

	for _, state := range rr.ReturnStates {
		if state == nil {
			continue
		}

		if state.StartTS.IsZero() && state.FinishTS.IsZero() {
			continue
		}

		if state.StartTS.After(state.FinishTS) {
			return xerrors.Errorf("state.StartTS.After(state.FinishTS): %s vs %s", state.StartTS, state.FinishTS)
		}
		if rr.StartTS.After(state.StartTS) {
			return xerrors.Errorf("rr.StartTS.After(state.StartTS): %s vs %s", rr.StartTS, state.StartTS)
		}
		if rr.FinishTS.Before(state.FinishTS) {
			return xerrors.Errorf("rr.FinishTS.Before(state.FinishTS): %s vs %s", rr.FinishTS, state.FinishTS)
		}
	}

	return nil
}

// returnResult is the raw form of job result
type returnResult struct {
	FQDN     string          `json:"id,omitempty"`
	Func     string          `json:"fun,omitempty"`
	FuncArgs []string        `json:"fun_args,omitempty"`
	JID      string          `json:"jid,omitempty"`
	Return   json.RawMessage `json:"return,omitempty"`
	// ReturnCode indicates if command execution was successful.
	ReturnCode int `json:"retcode,omitempty"`
	// Success indicates that command was executed. It does NOT indicate that command was successful.
	Success bool `json:"success,omitempty"`
}

// StateReturn is the result of one state run inside job
type StateReturn struct {
	Name     string                `json:"name" yaml:"name"`
	ID       string                `json:"id" yaml:"id"`
	Result   bool                  `json:"result" yaml:"result"`
	SLS      string                `json:"sls" yaml:"sls"`
	Changes  json.RawMessage       `json:"changes" yaml:"change"`
	Comment  StateReturnComment    `json:"comment" yaml:"comment"`
	Duration encodingutil.Duration `json:"duration" yaml:"duration"`

	// StartTS is calculated from JID and StartTime
	StartTS time.Time `json:"start_ts" yaml:"start_ts"`
	// FinishTS is calculated from JID, StartTime and Duration
	FinishTS time.Time `json:"finish_ts" yaml:"finish_ts"`
	// StartTime is the raw form state's start time
	StartTime string `json:"start_time" yaml:"start_time"`

	RunNum int `json:"run_num" yaml:"run_num"`
}

// stateReturn is the raw form of one state run inside job
type stateReturn struct {
	Name      string             `json:"name,omitempty"`
	ID        string             `json:"__id__,omitempty"`
	Result    bool               `json:"result"`
	SLS       string             `json:"__sls__,omitempty"`
	Changes   json.RawMessage    `json:"changes,omitempty"`
	Comment   StateReturnComment `json:"comment,omitempty"`
	Duration  float64            `json:"duration,omitempty"`
	StartTime string             `json:"start_time,omitempty"`
	RunNum    int                `json:"__run_num__,omitempty"`
}

// parseReturnResultBase takes in raw job result and converts it into strongly-typed model.
// Does not parse return value itself.
func parseReturnResultBase(data []byte) (ReturnResult, error) {
	var tmp returnResult
	if err := json.Unmarshal(data, &tmp); err != nil {
		return ReturnResult{}, xerrors.Errorf("failed to parse return: %w", err)
	}

	return ReturnResult{
		FQDN:       tmp.FQDN,
		Func:       tmp.Func,
		FuncArgs:   tmp.FuncArgs,
		JID:        tmp.JID,
		ReturnRaw:  tmp.Return,
		ReturnCode: tmp.ReturnCode,
		Success:    tmp.Success,
		ParseLevel: ParseLevelMinimal,
	}, nil
}

type ParseLevel string

const (
	ParseLevelNone    ParseLevel = "none"    // Nothing was parsed
	ParseLevelMinimal ParseLevel = "minimal" // Only top-level data was parsed without any details
	ParseLevelPartial ParseLevel = "partial" // Some details were parsed including basic states data
	ParseLevelFull    ParseLevel = "full"    // Everything was parsed
)

// ParseReturnResult takes in raw job result and converts it into strongly-typed model.
// Parses return value too. If return cannot be parsed, returns partially parsed result if able.
func ParseReturnResult(data []byte, tzOffset time.Duration) (ReturnResult, []error) {
	rr, errs := parseReturnResult(data, tzOffset)
	if rr.ParseLevel == ParseLevelNone {
		return rr, errs
	}

	if err := rr.validate(); err != nil {
		errs = append(errs, err)
	}

	return rr, errs
}

func parseReturnResult(data []byte, tzOffset time.Duration) (ReturnResult, []error) {
	rr, err := parseReturnResultBase(data)
	if err != nil {
		return rr, []error{err}
	}

	var rawStateReturns map[string]json.RawMessage
	if err := json.Unmarshal(rr.ReturnRaw, &rawStateReturns); err != nil {
		// This is a special case for simple commands like cmd.run/cmd.retcode
		if err = json.Unmarshal(rr.ReturnRaw, &rr.ReturnValue); err != nil {
			return rr, []error{xerrors.Errorf("failed to parse return into general value: %w", err)}
		}

		// Only JID left for 'string' return
		rr.ParseLevel = ParseLevelPartial
		if rr, err = parseAndApplyJID(rr); err != nil {
			return rr, []error{err}
		}

		// Fully parsed 'string' return
		rr.ParseLevel = ParseLevelFull
		return rr, nil
	}

	// We reached partial parsing
	rr.ParseLevel = ParseLevelPartial

	if rr, err = parseAndApplyJID(rr); err != nil {
		// Report states
		rr.ReturnStates = make(map[string]*StateReturn, len(rawStateReturns))
		for k := range rawStateReturns {
			rr.ReturnStates[k] = nil
		}

		return rr, nil
	}

	if len(rawStateReturns) != 0 {
		rr.ReturnStates = make(map[string]*StateReturn, len(rawStateReturns))
	}

	// Upgrade to full. We will downgrade later if we encounter any errors in states.
	rr.ParseLevel = ParseLevelFull

	var errs []error
	for k, v := range rawStateReturns {
		var parsedStateReturn stateReturn
		if err := json.Unmarshal(v, &parsedStateReturn); err != nil {
			errs = append(errs, xerrors.Errorf("failed to parse return state %q: %w", k, err))
			rr.ParseLevel = ParseLevelPartial
			rr.ReturnStates[k] = nil
			continue
		}

		state := StateReturn{
			Name:    parsedStateReturn.Name,
			ID:      parsedStateReturn.ID,
			Result:  parsedStateReturn.Result,
			SLS:     parsedStateReturn.SLS,
			Changes: parsedStateReturn.Changes,
			Comment: parsedStateReturn.Comment,
			// Duration is initially stored as milliseconds in floating point format
			Duration:  encodingutil.FromDuration(time.Microsecond * time.Duration(parsedStateReturn.Duration*1000)),
			StartTime: parsedStateReturn.StartTime,
			RunNum:    parsedStateReturn.RunNum,
		}

		// We might receive no time/duration at all. Handle it!
		// And in other places too...
		if state.StartTime == "" {
			rr.ReturnStates[k] = &state
			continue
		}

		stateStartTS, err := parseStartTime(rr.StartTS, parsedStateReturn.StartTime, tzOffset)
		if err != nil {
			errs = append(errs, xerrors.Errorf("invalid start time %q: %w", parsedStateReturn.StartTime, err))
			rr.ParseLevel = ParseLevelPartial
			rr.ReturnStates[k] = nil
			continue
		}

		// Save start timestamp
		state.StartTS = stateStartTS

		// Save finish timestamp. Also calculate finish timestamp of entire job.
		state.FinishTS = state.StartTS.Add(state.Duration.Duration)
		if rr.FinishTS.Before(state.FinishTS) {
			rr.FinishTS = state.FinishTS
		}

		rr.ReturnStates[k] = &state
	}

	return rr, errs
}

func parseAndApplyJID(rr ReturnResult) (ReturnResult, error) {
	startTS, err := parseJID(rr.JID)
	if err != nil {
		return rr, xerrors.Errorf("failed to parse JID %q: %w", rr.JID, err)
	}

	rr.StartTS = startTS
	rr.FinishTS = rr.StartTS
	return rr, nil
}

func parseJID(jid string) (time.Time, error) {
	if len(jid) != len("20191231135531678170") {
		return time.Time{}, xerrors.Errorf("jid %q length expected to be %d but it is %d", jid, len("20191231135531678170"), len(jid))
	}

	// JIDs use UTC - https://github.com/saltstack/salt/issues/33309
	formattedValue := fmt.Sprintf("%s-%s-%sT%s:%s:%s.%s000", jid[:4], jid[4:6], jid[6:8], jid[8:10], jid[10:12], jid[12:14], jid[14:20])
	ts, err := time.ParseInLocation("2006-01-02T15:04:05.999999999", formattedValue, time.UTC)
	if err != nil {
		return time.Time{}, xerrors.Errorf("failed to parse full timestamp %q: %w", formattedValue, err)
	}

	return ts, nil
}

func dateFromTime(t time.Time) time.Time {
	return time.Date(t.Year(), t.Month(), t.Day(), 0, 0, 0, 0, t.Location())
}

func parseStartTime(jobStartTime time.Time, stateStartTimeString string, tzOffset time.Duration) (time.Time, error) {
	// Leave only date
	stateStartDate := dateFromTime(jobStartTime)

	// Format is "13:56:19.018018"
	splits := strings.Split(stateStartTimeString, ":")
	if len(splits) < 3 {
		return time.Time{}, xerrors.Errorf("start time has %d time points instead of >= 3", len(splits))
	}

	stateStartTime, err := parseStartTimePoint(0, splits[0], time.Hour)
	if err != nil {
		return time.Time{}, xerrors.Errorf("invalid start time hours %q: %w", splits[0], err)
	}

	stateStartTime, err = parseStartTimePoint(stateStartTime, splits[1], time.Minute)
	if err != nil {
		return time.Time{}, xerrors.Errorf("invalid start time minutes %q: %w", splits[1], err)
	}

	// Split seconds and microseconds
	splits = strings.Split(splits[2], ".")

	// Seconds are now at zero
	stateStartTime, err = parseStartTimePoint(stateStartTime, splits[0], time.Second)
	if err != nil {
		return time.Time{}, xerrors.Errorf("invalid start time seconds %q: %w", splits[0], err)
	}

	// Do we have microseconds? This is actually a precaution, we've never seen this without microseconds
	if len(splits) > 1 {
		stateStartTime, err = parseStartTimePoint(stateStartTime, splits[1], time.Microsecond)
		if err != nil {
			return time.Time{}, xerrors.Errorf("invalid start time microseconds %q: %w", splits[1], err)
		}
	}

	// Form state's start timestamp
	stateStartTS := stateStartDate.Add(stateStartTime)

	// Roll day if state's start time is 'before' job's start time (it means state started the next day)
	if jobStartTime.Sub(stateStartDate) > stateStartTime {
		stateStartTS = stateStartTS.Add(24 * time.Hour)
	}

	// startTime is in local TZ, startDate is in UTC. We need to apply TZ offset to set correct startDate in UTC.
	stateStartTS = stateStartTS.Add(-tzOffset)
	return stateStartTS, nil
}

func parseStartTimePoint(d time.Duration, p string, mul time.Duration) (time.Duration, error) {
	i, err := strconv.ParseInt(p, 10, 64)
	if err != nil {
		return 0, err
	}

	return d + mul*time.Duration(i), nil
}

const (
	stateSLSFunc = "state.sls"
)

const (
	returnCodeOK                 = 0
	returnCodeCompilerError      = 1 // 1 is set when any error is encountered in the state compiler (missing SLS file, etc.)
	returnCodeResultError        = 2 // 2 is set when any state returns a False result
	returnCodePillarCompileError = 5 // 5 is set when Pillar data fails to be compiled before running the state(s)
)

// CanCheckSuccess returns true if result can be checked for success (parse level is adequate)
func (rr ReturnResult) CanCheckSuccess() bool {
	return rr.ParseLevel == ParseLevelPartial || rr.ParseLevel == ParseLevelFull
}

// IsSuccessful returns true if result is successful overall (command was executed and it was executed successfully).
func (rr ReturnResult) IsSuccessful() bool {
	if !rr.CanCheckSuccess() {
		panic(fmt.Sprintf("need at least partial parse to determine success, current level for jid %q is %q", rr.JID, rr.ParseLevel))
	}

	// Command failed?
	if !rr.Success {
		return false
	}

	switch rr.ReturnCode {
	case returnCodeOK:
		return true
	case returnCodeResultError:
		// There is a special case for 'state.sls' with zero return states
		// Its SaltStack, don't ask...
		if rr.Func == stateSLSFunc && len(rr.ReturnStates) == 0 {
			return true
		}

		return false
	default:
		return false
	}
}

// IsTestRun returns true if the result is job with test=True argument
func (rr ReturnResult) IsTestRun() bool {
	return slices.ContainsString(rr.FuncArgs, "test=True")
}

type StateReturnComment string

func (c StateReturnComment) MarshalJSON() ([]byte, error) {
	if _, err := strconv.ParseBool(string(c)); err != nil {
		return json.Marshal(string(c))
	}

	return []byte(string(c)), nil
}

func (c *StateReturnComment) UnmarshalJSON(data []byte) error {
	var s string
	if err := json.Unmarshal(data, &s); err != nil {
		var b bool
		if err = json.Unmarshal(data, &b); err != nil {
			return xerrors.New("state return comment is neither string nor bool")
		}

		*c = StateReturnComment(strconv.FormatBool(b))
		return nil
	}

	*c = StateReturnComment(s)
	return nil
}

func (c StateReturnComment) MarshalYAML() (interface{}, error) {
	return []byte(string(c)), nil
}
