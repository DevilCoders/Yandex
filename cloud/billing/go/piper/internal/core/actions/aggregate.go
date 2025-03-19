package actions

import (
	"context"
	"time"

	"github.com/mailru/easyjson"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/types"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/library/go/core/log"
)

//go:generate easyjson -disable_members_unescape -disallow_unknown_fields .

// Aggregate iterates over metrics and group by key which depends on schema.
// The main reason is reducing the number of metrics in output when it is possible.
func Aggregate(ctx context.Context, metrics []entities.SourceMetric) (result []entities.SourceMetric) {

	ctx = tooling.ActionStarted(ctx)
	logger := tooling.Logger(ctx)
	defer func() { tooling.ActionDone(ctx, nil) }()

	// Metrics are grouped by `key` into maps
	var storageGroups = make(map[s3RequestMetricsGroupKey]*s3RequestMetricsGroup)
	var dnsGroups = make(map[dnsRequestMetricsGroupKey]*dnsRequestMetricsGroup)

	aggregationSkipStats := map[string]*aggregationSkipsStats{}

	for i := range metrics {
		var err error
		switch metrics[i].Schema {
		case "s3.api.v1":
			var newGroup s3RequestMetricsGroup
			newGroup, err = newS3RequestMetricsGroup(&metrics[i])
			if err == nil {
				if existingGroup, ok := storageGroups[newGroup.Key()]; ok {
					existingGroup.Merge(newGroup)
				} else {
					storageGroups[newGroup.Key()] = &newGroup
				}
			}
		case "dns.requests.v1":
			var newGroup dnsRequestMetricsGroup
			newGroup, err = newDNSRequestMetricsGroup(&metrics[i])
			if err == nil {
				if existingGroup, ok := dnsGroups[newGroup.Key()]; ok {
					existingGroup.Merge(newGroup)
				} else {
					dnsGroups[newGroup.Key()] = &newGroup
				}
			}
		default:
			result = append(result, metrics[i])
		}
		if err != nil {
			result = append(result, metrics[i])
			if aggregationSkips, ok := aggregationSkipStats[metrics[i].Schema]; ok {
				aggregationSkips.count += 1
				aggregationSkips.reason = err
			} else {
				aggregationSkipStats[metrics[i].Schema] = &aggregationSkipsStats{1, err}
			}
		}
	}
	for key := range storageGroups {
		result = append(result, storageGroups[key].Result())
	}
	for key := range dnsGroups {
		result = append(result, dnsGroups[key].Result())
	}
	for key := range aggregationSkipStats {
		stats := aggregationSkipStats[key]
		logger.Error("skipping metric aggregation", log.String("schema", key), log.Int("count", stats.count), log.Error(stats.reason))
	}
	logger.Debug("number of reduced metrics", logf.Size(len(metrics)-len(result)))
	return
}

// s3RequestMetricsGroupKey describes a key to group metrics with `s3.api.v1` schema
type s3RequestMetricsGroupKey struct {
	resourceID    string
	schema        string
	folderID      string
	cloudID       string
	endTime       time.Time
	storageClass  string
	method        string
	statusCode    string
	netType       string
	messageOffset uint64
}

// s3RequestMetricsGroup represents a group of metrics with same key and sum(values)
type s3RequestMetricsGroup struct {
	// Origin metric
	metric *entities.SourceMetric
	key    s3RequestMetricsGroupKey
	tags   s3RequestTagsSchema
	// Values to group by
	usageQuantity   decimal.Decimal128
	usageStart      time.Time
	usageFinish     time.Time
	tagsTransferred uint64
}

func (s *s3RequestMetricsGroup) Key() s3RequestMetricsGroupKey {
	return s.key
}

func (s *s3RequestMetricsGroup) Merge(group s3RequestMetricsGroup) {
	s.usageQuantity = s.usageQuantity.Add(group.usageQuantity)
	s.tagsTransferred = s.tagsTransferred + group.tagsTransferred
	s.usageFinish = maxTime(s.usageFinish, group.usageFinish)
	s.usageStart = s.usageFinish
}

func (s s3RequestMetricsGroup) Result() entities.SourceMetric {
	metricCopy := s.metric.Clone()
	tags := s3RequestTagsSchema{
		Handler:      s.tags.Handler,
		StorageClass: s.tags.StorageClass,
		StatusCode:   s.tags.StatusCode,
		Transferred:  s.tagsTransferred,
		Method:       s.tags.Method,
		NetType:      s.tags.NetType,
	}
	tagsJSON, _ := tags.MarshalJSON()
	metricCopy.Usage.Quantity = s.usageQuantity
	metricCopy.Usage.Start = s.usageStart
	metricCopy.Usage.Finish = s.usageFinish
	metricCopy.Tags = types.JSONAnything(tagsJSON)
	return metricCopy
}

func newS3RequestMetricsGroup(metric *entities.SourceMetric) (s3RequestMetricsGroup, error) {
	var tags s3RequestTagsSchema
	if metric.Tags != "" {
		err := easyjson.Unmarshal([]byte(metric.Tags), &tags)
		if err != nil {
			return s3RequestMetricsGroup{}, err
		}
	}
	uh := usageHours(metric.Usage)
	groupKey := s3RequestMetricsGroupKey{
		resourceID:    metric.ResourceID,
		schema:        metric.Schema,
		folderID:      metric.FolderID,
		cloudID:       metric.CloudID,
		endTime:       uh.Finish,
		storageClass:  tags.StorageClass,
		method:        tags.Method,
		statusCode:    tags.StatusCode,
		netType:       tags.NetType,
		messageOffset: metric.MessageOffset,
	}
	group := s3RequestMetricsGroup{metric: metric, key: groupKey, tags: tags}
	group.usageQuantity = group.metric.Usage.Quantity
	group.tagsTransferred = tags.Transferred
	group.usageStart = group.metric.Usage.Start
	group.usageFinish = group.metric.Usage.Finish
	return group, nil
}

// s3RequestMetricsGroupKey describes a key to group metrics with `dns.requests.v1` schema
type dnsRequestMetricsGroupKey struct {
	schema        string
	folderID      string
	cloudID       string
	endTime       time.Time
	requestType   string
	forwarding    bool
	messageOffset uint64
}

// dnsRequestMetricsGroup represents a group of metrics with same key and sum(values)
type dnsRequestMetricsGroup struct {
	// Origin metric
	metric *entities.SourceMetric
	key    dnsRequestMetricsGroupKey
	// Values to group by
	usageQuantity decimal.Decimal128
	usageStart    time.Time
	usageFinish   time.Time
}

func (s dnsRequestMetricsGroup) Key() dnsRequestMetricsGroupKey {
	return s.key
}

func (s *dnsRequestMetricsGroup) Merge(group dnsRequestMetricsGroup) {
	s.usageQuantity = s.usageQuantity.Add(group.usageQuantity)
	s.usageFinish = maxTime(s.usageFinish, group.usageFinish)
	s.usageStart = s.usageFinish
}

func (s dnsRequestMetricsGroup) Result() entities.SourceMetric {
	metricCopy := s.metric.Clone()
	metricCopy.Usage.Quantity = s.usageQuantity
	metricCopy.Usage.Start = s.usageStart
	metricCopy.Usage.Finish = s.usageFinish
	return metricCopy
}

func newDNSRequestMetricsGroup(metric *entities.SourceMetric) (dnsRequestMetricsGroup, error) {
	var tags dnsRequestTagsSchema
	if metric.Tags != "" {
		err := easyjson.Unmarshal([]byte(metric.Tags), &tags)
		if err != nil {
			return dnsRequestMetricsGroup{}, err
		}
	}
	uh := usageHours(metric.Usage)
	groupKey := dnsRequestMetricsGroupKey{
		schema:        metric.Schema,
		folderID:      metric.FolderID,
		cloudID:       metric.CloudID,
		messageOffset: metric.MessageOffset,
		endTime:       uh.Finish,
		requestType:   tags.RequestType,
		forwarding:    tags.Forwarding,
	}
	group := dnsRequestMetricsGroup{metric: metric, key: groupKey}
	group.usageQuantity = group.metric.Usage.Quantity
	group.usageStart = group.metric.Usage.Start
	group.usageFinish = group.metric.Usage.Finish
	return group, nil
}

//easyjson:json
type s3RequestTagsSchema struct {
	Handler      string `json:"handler,omitempty"` // Get Object (text description of method, ignore it)
	StorageClass string `json:"storage_class"`     // COLD
	Method       string `json:"method"`            // GET
	StatusCode   string `json:"status_code"`       // 404
	NetType      string `json:"net_type"`          // cloud
	Transferred  uint64 `json:"transferred"`       // 476 (bytes)
}

//easyjson:json
type dnsRequestTagsSchema struct {
	RequestType string `json:"request_type"` // PUBLIC
	Forwarding  bool   `json:"forwarding"`   // true
}

type aggregationSkipsStats struct {
	count  int
	reason error
}
