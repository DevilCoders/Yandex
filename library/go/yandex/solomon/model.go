package solomon

import (
	"encoding/json"
	"fmt"

	"github.com/gofrs/uuid"
	"github.com/google/go-cmp/cmp"
	"github.com/google/go-cmp/cmp/cmpopts"

	"a.yandex-team.ru/library/go/x/encoding/unknownjson"
)

type AggregationRule struct {
	Conditions []string `json:"cond"`
	Targets    []string `json:"target"`
}

type Service struct {
	ID               string `json:"id"`
	Name             string `json:"name"`
	Port             int    `json:"port"`
	Path             string `json:"path"`
	AddTimestampArgs *bool  `json:"addTsArgs,omitempty"`
	Interval         int    `json:"interval,omitempty"`
	Grid             int    `json:"gridSec,omitempty"`
	SensorTTLDays    int    `json:"sensorsTtlDays,omitempty"`
	Version          int    `json:"version"`

	Sensors struct {
		AggregationRules []AggregationRule `json:"aggrRules"`
	} `json:"sensorConf"`

	Unrecognized unknownjson.Store `json:"-" unknown:",store"`
}

func (s Service) DiffKey() string { return s.ID }

func (s *Service) MarshalJSON() ([]byte, error) {
	type service Service
	return unknownjson.Marshal(service(*s))
}

func (s *Service) UnmarshalJSON(data []byte) error {
	type service Service
	return unknownjson.Unmarshal(data, (*service)(s))
}

func (s Service) IsEqual(other Service) bool {
	return cmp.Equal(s, other,
		cmpopts.EquateEmpty(),
		cmpopts.IgnoreFields(Service{}, "Version", "Unrecognized"))
}

func (s *Service) GenerateID() {
	if s.ID == "" {
		s.ID = uuid.Must(uuid.NewV4()).String()
	}
}

type NannyGroup struct {
	Service      string   `json:"service"`
	GencfgGroups []string `json:"cfgGroup"`
	Labels       []string `json:"labels"`
}

// MarshalJSON is workaround for SOLOMON-6390
func (n *NannyGroup) MarshalJSON() ([]byte, error) {
	type nannyGroup NannyGroup
	nn := nannyGroup(*n)

	if nn.GencfgGroups == nil {
		nn.GencfgGroups = []string{}
	}

	if nn.Labels == nil {
		nn.Labels = []string{}
	}

	return json.Marshal(nn)
}

type HostURL struct {
	URL string `json:"url"`
}

type ConductorGroup struct {
	Group string `json:"group"`
}

type YPCluster struct {
	PodSetID string `json:"podSetId"`
	Cluster  string `json:"cluster"`
}

type Cluster struct {
	ID      string `json:"id"`
	Name    string `json:"name"`
	Version int    `json:"version"`

	NannyGroups     []NannyGroup     `json:"nannyGroups,omitempty"`
	HostURLs        []HostURL        `json:"hostUrls,omitempty"`
	ConductorGroups []ConductorGroup `json:"conductorGroups,omitempty"`
	YPClusters      []YPCluster      `json:"ypClusters,omitempty"`

	Unrecognized unknownjson.Store `json:"-" unknown:",store"`
}

func (c Cluster) DiffKey() string { return c.ID }

func (c *Cluster) MarshalJSON() ([]byte, error) {
	type cluster Cluster
	return unknownjson.Marshal(cluster(*c))
}

func (c *Cluster) UnmarshalJSON(data []byte) error {
	type cluster Cluster
	return unknownjson.Unmarshal(data, (*cluster)(c))
}

func (c Cluster) IsEqual(other Cluster) bool {
	return cmp.Equal(c, other,
		cmpopts.EquateEmpty(),
		cmpopts.IgnoreFields(Cluster{}, "Version", "Unrecognized"))
}

func (c *Cluster) GenerateID() {
	if c.ID == "" {
		c.ID = uuid.Must(uuid.NewV4()).String()
	}
}

type Shard struct {
	ID          string `json:"id,omitempty"`
	ClusterID   string `json:"clusterId,omitempty"`
	ServiceID   string `json:"serviceId,omitempty"`
	ClusterName string `json:"clusterName"`
	ServiceName string `json:"serviceName"`
	Version     int    `json:"version"`

	State string `json:"state,omitempty"`

	Unrecognized unknownjson.Store `json:"-" unknown:",store"`
}

func (s Shard) DiffKey() string {
	return fmt.Sprintf("%s;%s", s.ClusterID, s.ServiceID)
}

func (s *Shard) MarshalJSON() ([]byte, error) {
	type shard Shard
	return unknownjson.Marshal(shard(*s))
}

func (s *Shard) UnmarshalJSON(data []byte) error {
	type shard Shard
	return unknownjson.Unmarshal(data, (*shard)(s))
}

func (s Shard) IsEqual(other Shard) bool {
	return cmp.Equal(s, other,
		cmpopts.EquateEmpty(),
		cmpopts.IgnoreFields(Shard{}, "Version", "Unrecognized"))
}

func (s *Shard) GenerateID() {
	if s.ID == "" {
		s.ID = uuid.Must(uuid.NewV4()).String()
	}
}

type ClusterService struct {
	ID      string `json:"id"`
	State   string `json:"state"`
	ShardID string `json:"shardId"`
	Version int    `json:"version"`
}

type HostStatus struct {
	Host string `json:"host"`
	URL  string `json:"url"`

	LastStatus string `json:"lastStatus"`
	LastError  string `json:"lastError"`
}

type AlertDescription struct {
	ID    string `json:"id"`
	Name  string `json:"name"`
	Type  string `json:"type"`
	State string `json:"state"`
}

type AlertPredicateRule struct {
	ThresholdType string  `json:"thresholdType"`
	Comparison    string  `json:"comparison"`
	Threshold     float64 `json:"threshold"`
	TargetStatus  string  `json:"targetStatus"`
}

type AlertThreshold struct {
	Selectors      string               `json:"selectors"`
	PredicateRules []AlertPredicateRule `json:"predicateRules"`
}

type AlertExpression struct {
	Program         string `json:"program"`
	CheckExpression string `json:"checkExpression,omitempty"`
}

type AlertType struct {
	Threshold  *AlertThreshold  `json:"threshold,omitempty"`
	Expression *AlertExpression `json:"expression,omitempty"`
}

type Annotations struct {
	Host        string `json:"host,omitempty"`
	Tag         string `json:"tag,omitempty"`
	Service     string `json:"service,omitempty"`
	Description string `json:"description,omitempty"`
}

type ChannelConfig struct {
	NotifyAboutStatuses []string `json:"notifyAboutStatuses"`
	RepeatDelaySecs     int64    `json:"repeatDelaySecs"`
}

type AssociatedChannel struct {
	ID     string        `json:"id"`
	Config ChannelConfig `json:"config"`
}

type Alert struct {
	ProjectID            string              `json:"projectId"`
	ID                   string              `json:"id"`
	Name                 string              `json:"name"`
	Channels             []AssociatedChannel `json:"channels,omitempty"`
	Description          string              `json:"description,omitempty"`
	Version              int                 `json:"version,omitempty"`
	State                string              `json:"state,omitempty"`
	GroupByLabels        []string            `json:"groupByLabels,omitempty"`
	NotificationChannels []string            `json:"notificationChannels,omitempty"`
	Type                 AlertType           `json:"type,omitempty"`
	Annotations          Annotations         `json:"annotations,omitempty"`
	PeriodMillis         int                 `json:"periodMillis,omitempty"`
	WindowSeconds        int                 `json:"windowSecs,omitempty"`
	DelaySecs            int                 `json:"delaySecs,omitempty"`
	DelaySeconds         int                 `json:"delaySeconds,omitempty"`
	NoPointsPolicy       string              `json:"noPointsPolicy,omitempty"`
}

func (a Alert) IsEqual(other Alert) bool {
	return cmp.Equal(a, other,
		cmpopts.EquateEmpty(),
		cmpopts.IgnoreFields(Alert{}, "Version"))
}

type DashboardParameter struct {
	Name  string `json:"name"`
	Value string `json:"value"`
}

type DashboardPanel struct {
	Type     string `json:"type"`
	Title    string `json:"title"`
	SubTitle string `json:"subtitle"`
	URL      string `json:"url"`
	Markdown string `json:"markdown"`
	RowSpan  int    `json:"rowspan,omitempty"`
	ColSpan  int    `json:"colspan,omitempty"`
}

type DashboardRow struct {
	Panels []DashboardPanel `json:"panels"`
}

type Dashboard struct {
	ProjectID string `json:"projectId"`
	ID        string `json:"id"`
	Name      string `json:"name"`
	Version   int    `json:"version"`

	Description      string               `json:"description"`
	HeightMultiplier float64              `json:"heightMultiplier"`
	Parameters       []DashboardParameter `json:"parameters"`
	Rows             []DashboardRow       `json:"rows"`
}
