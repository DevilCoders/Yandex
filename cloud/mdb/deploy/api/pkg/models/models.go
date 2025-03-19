package models

import (
	"encoding/json"
	"strconv"
	"strings"
	"time"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
)

// GroupID is deploy group ID
type GroupID int64

func (id GroupID) String() string {
	return strconv.FormatInt(int64(id), 10)
}

// Group describes group for salt masters and minions
type Group struct {
	ID   GroupID
	Name string
}

// MasterID is salt master's internal ID (never shown to service' clients)
type MasterID int64

func (id MasterID) String() string {
	return strconv.FormatInt(int64(id), 10)
}

// MinionID is salt minions's internal ID (never shown to service' clients)
type MinionID int64

func (id MinionID) String() string {
	return strconv.FormatInt(int64(id), 10)
}

// Master describes salt master
type Master struct {
	ID           MasterID
	Aliases      []string
	AliveCheckAt time.Time
	CreatedAt    time.Time
	FQDN         string
	Group        string
	Description  string
	IsOpen       bool
	Alive        bool
	PublicKey    string
}

// Minion describes salt minion
type Minion struct {
	ID            MinionID
	FQDN          string
	Group         string
	MasterFQDN    string
	CreatedAt     time.Time
	UpdatedAt     time.Time
	RegisterUntil time.Time
	AutoReassign  bool
	Registered    bool
	Deleted       bool
	PublicKey     string
}

// ShipmentID is shipment's ID
type ShipmentID int64

func (id ShipmentID) String() string {
	return strconv.FormatInt(int64(id), 10)
}

// ParseShipmentID from string
func ParseShipmentID(str string) (ShipmentID, error) {
	id, err := strconv.ParseInt(str, 10, 64)
	return ShipmentID(id), err
}

// CommandDef ...
type CommandDef struct {
	Type    string
	Args    []string `json:",omitempty" yaml:",omitempty"`
	Timeout encodingutil.Duration
}

// Shipment describes shipment to run on multiple minions
type Shipment struct {
	ID               ShipmentID
	Commands         []CommandDef
	FQDNs            []string
	Status           ShipmentStatus
	Parallel         int64
	StopOnErrorCount int64
	OtherCount       int64
	DoneCount        int64
	ErrorsCount      int64
	TotalCount       int64
	CreatedAt        time.Time
	UpdatedAt        time.Time
	Timeout          encodingutil.Duration
	Tracing          opentracing.TextMapCarrier
}

// ShipmentStatus ...
type ShipmentStatus string

// MarshalText marshals shipment status to text
func (ss ShipmentStatus) MarshalText() ([]byte, error) {
	return []byte(ss), nil
}

// UnmarshalText unmarshals shipment status from text
func (ss *ShipmentStatus) UnmarshalText(text []byte) error {
	*ss = ParseShipmentStatus(string(text))
	return nil
}

// Known shipment statuses
const (
	ShipmentStatusUnknown    ShipmentStatus = "UNKNOWN"
	ShipmentStatusInProgress ShipmentStatus = "INPROGRESS"
	ShipmentStatusDone       ShipmentStatus = "DONE"
	ShipmentStatusError      ShipmentStatus = "ERROR"
	ShipmentStatusTimeout    ShipmentStatus = "TIMEOUT"
)

var shipmentStatusMap = map[ShipmentStatus]struct{}{
	ShipmentStatusUnknown:    {},
	ShipmentStatusInProgress: {},
	ShipmentStatusDone:       {},
	ShipmentStatusError:      {},
	ShipmentStatusTimeout:    {},
}

// ShipmentStatusList returns list of known shipment statuses
func ShipmentStatusList() []string {
	l := make([]string, 0, len(shipmentStatusMap))
	for k := range shipmentStatusMap {
		l = append(l, string(k))
	}

	return l
}

// ParseShipmentStatus from string
func ParseShipmentStatus(str string) ShipmentStatus {
	ss := ShipmentStatus(strings.ToUpper(str))
	if _, ok := shipmentStatusMap[ss]; !ok {
		return ShipmentStatusUnknown
	}

	return ss
}

// CommandID is command's ID
type CommandID int64

func (id CommandID) String() string {
	return strconv.FormatInt(int64(id), 10)
}

// ParseCommandID from string
func ParseCommandID(str string) (CommandID, error) {
	id, err := strconv.ParseInt(str, 10, 64)
	return CommandID(id), err
}

// Command describes command to run on minion
type Command struct {
	CommandDef

	ID         CommandID
	ShipmentID ShipmentID
	FQDN       string
	Status     CommandStatus
	CreatedAt  time.Time
	UpdatedAt  time.Time
}

// CommandStatus ...
type CommandStatus string

// MarshalText marshals command status to text
func (cs CommandStatus) MarshalText() ([]byte, error) {
	return []byte(cs), nil
}

// UnmarshalText unmarshals command status from text
func (cs *CommandStatus) UnmarshalText(text []byte) error {
	*cs = ParseCommandStatus(string(text))
	return nil
}

// Known command statuses
const (
	CommandStatusUnknown   CommandStatus = "UNKNOWN"
	CommandStatusAvailable CommandStatus = "AVAILABLE"
	CommandStatusRunning   CommandStatus = "RUNNING"
	CommandStatusDone      CommandStatus = "DONE"
	CommandStatusError     CommandStatus = "ERROR"
	CommandStatusCanceled  CommandStatus = "CANCELED"
	CommandStatusTimeout   CommandStatus = "TIMEOUT"
)

var commandStatusMap = map[CommandStatus]struct{}{
	CommandStatusUnknown:   {},
	CommandStatusAvailable: {},
	CommandStatusRunning:   {},
	CommandStatusDone:      {},
	CommandStatusError:     {},
	CommandStatusCanceled:  {},
	CommandStatusTimeout:   {},
}

// ParseCommandStatus from string
func ParseCommandStatus(str string) CommandStatus {
	cs := CommandStatus(strings.ToUpper(str))
	if _, ok := commandStatusMap[cs]; !ok {
		return CommandStatusUnknown
	}

	return cs
}

// JobID is job's unique ID
type JobID int64

func (id JobID) String() string {
	return strconv.FormatInt(int64(id), 10)
}

// ParseJobID from string
func ParseJobID(str string) (JobID, error) {
	id, err := strconv.ParseInt(str, 10, 64)
	return JobID(id), err
}

// Job describes job performing some command
type Job struct {
	ID        JobID
	ExtID     string
	CommandID CommandID
	Status    JobStatus
	CreatedAt time.Time
	UpdatedAt time.Time
}

// JobForTimeout describes job that should be timed out
type JobForTimeout struct {
	ExtID  string
	Minion string
	Master string
}

// JobStatus ...
type JobStatus string

// MarshalText marshals job status to text
func (js JobStatus) MarshalText() ([]byte, error) {
	return []byte(js), nil
}

// UnmarshalText unmarshals job status from text
func (js *JobStatus) UnmarshalText(text []byte) error {
	*js = ParseJobStatus(string(text))
	return nil
}

// Known job statuses
const (
	JobStatusUnknown JobStatus = "UNKNOWN"
	JobStatusRunning JobStatus = "RUNNING"
	JobStatusDone    JobStatus = "DONE"
	JobStatusError   JobStatus = "ERROR"
	JobStatusTimeout JobStatus = "TIMEOUT"
)

var jobStatusMap = map[JobStatus]struct{}{
	JobStatusUnknown: {},
	JobStatusRunning: {},
	JobStatusDone:    {},
	JobStatusError:   {},
}

// ParseJobStatus from string
func ParseJobStatus(str string) JobStatus {
	js := JobStatus(strings.ToUpper(str))
	if _, ok := jobStatusMap[js]; !ok {
		return JobStatusUnknown
	}

	return js
}

// JobResultStatus ...
type JobResultStatus string

// MarshalText marshals job result status to text
func (jrs JobResultStatus) MarshalText() ([]byte, error) {
	return []byte(jrs), nil
}

// UnmarshalText unmarshals job result status from text
func (jrs *JobResultStatus) UnmarshalText(text []byte) error {
	*jrs = ParseJobResultStatus(string(text))
	return nil
}

// Known job statuses
const (
	JobResultStatusUnknown    JobResultStatus = "UNKNOWN"
	JobResultStatusSuccess    JobResultStatus = "SUCCESS"
	JobResultStatusFailure    JobResultStatus = "FAILURE"
	JobResultStatusTimeout    JobResultStatus = "TIMEOUT"
	JobResultStatusNotRunning JobResultStatus = "NOTRUNNING"
)

var jobResultStatusMap = map[JobResultStatus]struct{}{
	JobResultStatusUnknown:    {},
	JobResultStatusSuccess:    {},
	JobResultStatusFailure:    {},
	JobResultStatusTimeout:    {},
	JobResultStatusNotRunning: {},
}

// JobResultStatusList returns list of known job result statuses
func JobResultStatusList() []string {
	l := make([]string, 0, len(jobResultStatusMap))
	for k := range jobResultStatusMap {
		l = append(l, string(k))
	}

	return l
}

// ParseJobResultStatus from string
func ParseJobResultStatus(str string) JobResultStatus {
	jrs := JobResultStatus(strings.ToUpper(str))
	if _, ok := jobResultStatusMap[jrs]; !ok {
		return JobResultStatusUnknown
	}

	return jrs
}

// JobResultID is job result's unique ID
type JobResultID int64

func (id JobResultID) String() string {
	return strconv.FormatInt(int64(id), 10)
}

// ParseJobResultID from string
func ParseJobResultID(str string) (JobResultID, error) {
	id, err := strconv.ParseInt(str, 10, 64)
	return JobResultID(id), err
}

// JobResult ...
type JobResult struct {
	JobResultID JobResultID
	ExtID       string
	FQDN        string
	Order       int
	Status      JobResultStatus
	Result      json.RawMessage
	RecordedAt  time.Time
}

// SortOrder for listings
type SortOrder string

// Possible sorting orders
const (
	SortOrderAsc     SortOrder = "asc"
	SortOrderDesc    SortOrder = "desc"
	SortOrderUnknown SortOrder = "unknown"
)

var sortOrderMap = map[SortOrder]struct{}{
	SortOrderAsc:     {},
	SortOrderDesc:    {},
	SortOrderUnknown: {},
}

// ParseSortOrder from string
func ParseSortOrder(str string) SortOrder {
	so := SortOrder(strings.ToLower(str))
	if _, ok := sortOrderMap[so]; !ok {
		return SortOrderUnknown
	}

	return so
}

type JobResultCoords struct {
	ShipmentID ShipmentID
	CommandID  CommandID
	JobID      JobID
	Tracing    opentracing.TextMapCarrier
}
