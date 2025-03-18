package nanny

import "a.yandex-team.ru/library/go/x/encoding/unknownjson"

type ChangeInfo struct {
	Comment string `json:"comment"`
	Author  string `json:"author"`
}

type SnapshotState string

const (
	StateActive    SnapshotState = "ACTIVE"
	StatePrepare   SnapshotState = "PREPARE"
	StateCreated   SnapshotState = "CREATED"
	StateDestroyed SnapshotState = "DESTROYED"
)

type TargetState struct {
	ID string `json:"_id,omitempty"`

	IsEnabled    bool   `json:"is_enabled"`
	SnapshotID   string `json:"snapshot_id"`
	SetAsCurrent *bool  `json:"set_as_current,omitempty"`
	TicketID     string `json:"ticket_id,omitempty"`

	PrepareRecipe string `json:"prepare_recipe,omitempty"`
	Recipe        string `json:"recipe,omitempty"`

	Comment string `json:"comment,omitempty"`

	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

func (s *TargetState) MarshalJSON() ([]byte, error) {
	type state TargetState
	return unknownjson.Marshal(state(*s))
}

func (s *TargetState) UnmarshalJSON(data []byte) error {
	type state TargetState
	return unknownjson.Unmarshal(data, (*state)(s))
}

type CurrentState struct {
	ID string `json:"_id,omitempty"`

	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

func (s *CurrentState) MarshalJSON() ([]byte, error) {
	type currentState CurrentState
	return unknownjson.Marshal(currentState(*s))
}

func (s *CurrentState) UnmarshalJSON(data []byte) error {
	type currentState CurrentState
	return unknownjson.Unmarshal(data, (*currentState)(s))
}

type InfoAttrs struct {
	ID string `json:"_id,omitempty"`

	Description string `json:"desc"`
	Category    string `json:"category"`

	ABCService int `json:"abc_group"`

	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

func (a *InfoAttrs) MarshalJSON() ([]byte, error) {
	type infoAttrs InfoAttrs
	return unknownjson.Marshal(infoAttrs(*a))
}

func (a *InfoAttrs) UnmarshalJSON(data []byte) error {
	type infoAttrs InfoAttrs
	return unknownjson.Unmarshal(data, (*infoAttrs)(a))
}

type SandboxFile struct {
	ResourceType   string `json:"resource_type,omitempty"`
	ResourceID     int    `json:"resource_id,string,omitempty"`
	TaskType       string `json:"task_type,omitempty"`
	TaskID         int    `json:"task_id,string,omitempty"`
	LocalPath      string `json:"local_path"`
	ExtractionPath string `json:"extraction_path,omitempty"`

	IsDynamic bool `json:"is_dynamic"`
}

type StaticFile struct {
	Content   string `json:"content"`
	LocalPath string `json:"local_path"`
	IsDynamic bool   `json:"is_dynamic"`
}

type InstanceType string

const (
	InstanceTypeGENCFG   = "EXTENDED_GENCFG_GROUPS"
	InstanceTypeYPPods   = "YP_PODS"
	InstanceTypeYPPodIDs = "YP_POD_IDS"
)

type SandboxResource struct {
	ResourceType string `json:"resourceType"`
	ResourceID   int    `json:"resourceId,string"`
	TaskType     string `json:"taskType"`
	TaskID       int    `json:"taskId,string"`
}

type Meta struct {
	Type            string           `json:"type"`
	SandboxResource *SandboxResource `json:"sandboxResource,omitempty"`
}

type Instances struct {
	Type InstanceType `json:"chosen_type"`

	GencfgGroups []GencfgGroup     `json:"gencfg_groups,omitempty"`
	InstanceList []ServiceInstance `json:"instance_list,omitempty"`

	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

type Engine struct {
	Type string `json:"engine_type"`

	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

var EngineYPLite = Engine{Type: "YP_LITE"}

type GencfgGroup struct {
	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

type ServiceInstance struct {
	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

type RuntimeAttrs struct {
	ID string `json:"_id,omitempty"`

	Resources    Resources    `json:"resources"`
	InstanceSpec InstanceSpec `json:"instance_spec"`
	Instances    Instances    `json:"instances"`
	Engine       Engine       `json:"engines"`

	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

func (a *RuntimeAttrs) MarshalJSON() ([]byte, error) {
	type runtimeAttrs RuntimeAttrs
	return unknownjson.Marshal(runtimeAttrs(*a))
}

func (a *RuntimeAttrs) UnmarshalJSON(data []byte) error {
	type runtimeAttrs RuntimeAttrs
	return unknownjson.Unmarshal(data, (*runtimeAttrs)(a))
}

type UserSet struct {
	Logins []string `json:"logins,omitempty"`
	Groups []string `json:"groups,omitempty"`

	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

type AuthAttrs struct {
	Owners       UserSet `json:"owners"`
	Observers    UserSet `json:"observers"`
	ConfManagers UserSet `json:"conf_managers"`
	OpsManagers  UserSet `json:"ops_managers"`

	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

func (a *AuthAttrs) MarshalJSON() ([]byte, error) {
	type authAttrs AuthAttrs
	return unknownjson.Marshal(authAttrs(*a))
}

func (a *AuthAttrs) UnmarshalJSON(data []byte) error {
	type authAttrs AuthAttrs
	return unknownjson.Unmarshal(data, (*authAttrs)(a))
}

type ServiceSpec struct {
	ID           string
	InfoAttrs    *InfoAttrs
	RuntimeAttrs *RuntimeAttrs
	AuthAttrs    *AuthAttrs
}

type Service struct {
	ID string `json:"_id"`

	Info    *InfoSnapshot    `json:"info_attrs,omitempty"`
	Runtime *RuntimeSnapshot `json:"runtime_attrs,omitempty"`
	Auth    *AuthSnapshot    `json:"auth_attrs,omitempty"`

	TargetState  *TargetState  `json:"target_state,omitempty"`
	CurrentState *CurrentState `json:"current_state,omitempty"`
}

type InfoSnapshot struct {
	ID         string     `json:"_id,omitempty"`
	ChangeInfo ChangeInfo `json:"change_info"`
	Attrs      *InfoAttrs `json:"content"`
}

func (s *InfoSnapshot) UpdateWithComment(msg string) *UpdateInfo {
	return &UpdateInfo{Comment: msg, OldSnapshotID: s.ID}
}

type RuntimeSnapshot struct {
	ID         string        `json:"_id,omitempty"`
	ChangeInfo ChangeInfo    `json:"change_info"`
	Attrs      *RuntimeAttrs `json:"content"`
}

func (s *RuntimeSnapshot) UpdateWithComment(msg string) *UpdateInfo {
	return &UpdateInfo{Comment: msg, OldSnapshotID: s.ID}
}

type AuthSnapshot struct {
	ID         string     `json:"_id,omitempty"`
	ChangeInfo ChangeInfo `json:"change_info"`
	Attrs      *AuthAttrs `json:"content"`
}

func (s *AuthSnapshot) UpdateWithComment(msg string) *UpdateInfo {
	return &UpdateInfo{Comment: msg, OldSnapshotID: s.ID}
}

type UpdateInfo struct {
	OldSnapshotID string
	Comment       string
}

func (u *UpdateInfo) Commit(content interface{}) *commit {
	return &commit{
		Comment:    u.Comment,
		SnapshotID: u.OldSnapshotID,
		Content:    content,
	}
}

type commit struct {
	Comment    string      `json:"comment"`
	SnapshotID string      `json:"snapshot_id"`
	Content    interface{} `json:"content"`
}
