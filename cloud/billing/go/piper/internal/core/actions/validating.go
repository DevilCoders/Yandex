package actions

import (
	"context"
	"fmt"
	"sort"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/types"
	"a.yandex-team.ru/library/go/core/log"
)

type SchemaGetter interface {
	// NOTE: assume return empty MetricsSchema if no schema in storage
	// should be heavily cached or change to batch request
	GetMetricSchema(
		ctx context.Context, scope entities.ProcessingScope, schemaName string,
	) (schema entities.MetricsSchema, err error)
}

type DuplicatesSeeker interface {
	FindDuplicates(
		ctx context.Context, scope entities.ProcessingScope, periodEnd time.Time, ids []entities.MetricIdentity,
	) (duplicates []entities.MetricIdentity, err error)
}

func ValidateMetricsSchema(
	ctx context.Context, scope entities.ProcessingScope, sg SchemaGetter,
	metrics []entities.SourceMetric,
) (valid []entities.SourceMetric, invalid []entities.InvalidMetric, err error) {
	ctx = tooling.ActionStarted(ctx)
	defer func() {
		tooling.ActionDone(ctx, err)
		if err == nil {
			tooling.InvalidMetrics(ctx, invalid)
		}
	}()

	v := metricTagsValidator{ctx: ctx, scope: scope, sg: sg}
	return v.validate(metrics)
}

func ValidateMetricsModel(
	ctx context.Context, scope entities.ProcessingScope,
	metrics []entities.SourceMetric,
) (valid []entities.SourceMetric, invalid []entities.InvalidMetric, err error) {
	ctx = tooling.ActionStarted(ctx)
	defer func() {
		tooling.ActionDone(ctx, err)
		if err == nil {
			tooling.InvalidMetrics(ctx, invalid)
		}
	}()

	v := metricValuesValidator{ctx: ctx}
	return v.validateModel(metrics)
}

func ValidateMetricsQuantity(
	ctx context.Context, scope entities.ProcessingScope,
	metrics []entities.SourceMetric,
) (valid []entities.SourceMetric, invalid []entities.InvalidMetric, err error) {
	ctx = tooling.ActionStarted(ctx)
	defer func() {
		tooling.ActionDone(ctx, err)
		if err == nil {
			tooling.InvalidMetrics(ctx, invalid)
		}
	}()

	v := metricValuesValidator{ctx: ctx}
	return v.validatePositiveQty(metrics)
}

func ValidateMetricsWriteTime(
	ctx context.Context, scope entities.ProcessingScope, lifetime time.Duration,
	metrics []entities.SourceMetric,
) (valid []entities.SourceMetric, invalid []entities.InvalidMetric, err error) {
	ctx = tooling.ActionStarted(ctx)
	defer func() {
		tooling.ActionDone(ctx, err)
		if err == nil {
			tooling.InvalidMetrics(ctx, invalid)
		}
	}()

	v := metricValuesValidator{ctx: ctx}
	return v.validateWriteLag(metrics, lifetime)
}

func ValidateMetricsGrace(
	ctx context.Context, scope entities.ProcessingScope, grace time.Duration,
	metrics []entities.SourceMetric,
) (valid []entities.SourceMetric, invalid []entities.InvalidMetric, err error) {
	ctx = tooling.ActionStarted(ctx)
	defer func() {
		tooling.ActionDone(ctx, err)
		if err == nil {
			tooling.InvalidMetrics(ctx, invalid)
		}
	}()
	graceTime := billGraceTime(tooling.LocalTime(ctx, scope.StartTime), grace)
	v := metricValuesValidator{ctx: ctx}
	return v.validateWriteGrace(metrics, graceTime)
}

func ValidateMetricsUnique(
	ctx context.Context, scope entities.ProcessingScope, seeker DuplicatesSeeker, metrics []entities.SourceMetric,
) (valid []entities.SourceMetric, duplicates []entities.InvalidMetric, err error) {
	ctx = tooling.ActionStarted(ctx)
	defer func() {
		tooling.ActionDone(ctx, err)
	}()

	v := metricsUniquenessValidator{
		ctx:    ctx,
		scope:  scope,
		seeker: seeker,
	}

	return v.validate(metrics)
}

type metricTagsValidator struct {
	validatorCommon

	ctx   context.Context
	scope entities.ProcessingScope
	sg    SchemaGetter
}

func (v *metricTagsValidator) validate(
	metrics []entities.SourceMetric,
) (valid []entities.SourceMetric, invalid []entities.InvalidMetric, err error) {
	schemas := make(map[string]entities.MetricsSchema)
	valid = make([]entities.SourceMetric, 0, len(metrics))

	logger := tooling.Logger(v.ctx)
	for _, m := range metrics {
		schema, ok := schemas[m.Schema]
		if !ok {
			if schemas[m.Schema], err = v.sg.GetMetricSchema(v.ctx, v.scope, m.Schema); err != nil {
				logger.Error(
					"get schema error", logf.Error(err), logf.ErrorValue(m.Schema), logf.Offset(m.MessageOffset),
				)
				return
			}
			schema = schemas[m.Schema]
		}

		if schema.Empty() {
			logger.Debug(
				"no schema for metric", logf.ErrorKey("schema"), logf.ErrorValue(m.Schema), logf.Offset(m.MessageOffset),
			)
			invalid = append(invalid, v.makeIncorrectMetric(
				m, entities.FailedByInvalidTags, fmt.Sprintf("schema '%s' is not found", m.Schema),
			))
			continue
		}

		if missedTag := v.checkTags(m.Tags, schema.RequiredTags); missedTag != "" {
			logger.Debug(
				"required tag missed", logf.ErrorKey("tag"), logf.ErrorValue(missedTag), logf.Offset(m.MessageOffset),
			)
			invalid = append(invalid, v.makeIncorrectMetric(
				m, entities.FailedByInvalidTags, fmt.Sprintf("required tag '%s' missed", missedTag),
			))
			continue
		}
		valid = append(valid, m)
	}
	return
}

func (v *metricTagsValidator) checkTags(tags types.JSONAnything, required []string) string {
	if len(required) == 0 {
		return ""
	}
	parser := parsers.Get()
	defer parsers.Put(parser)

	val, _ := parser.Parse(string(tags)) // tags are empty or valid json and both cases are good for this check

	for _, t := range required {
		if val.Get(t) == nil {
			return t
		}
	}
	return ""
}

type metricValuesValidator struct {
	validatorCommon

	ctx context.Context
}

func (v *metricValuesValidator) validateModel(
	metrics []entities.SourceMetric,
) (valid []entities.SourceMetric, invalid []entities.InvalidMetric, err error) {
	logger := tooling.Logger(v.ctx)
	valid = make([]entities.SourceMetric, 0, len(metrics))
	for _, m := range metrics {
		if m.Version == "" {
			logger.Debug("empty version", logf.Offset(m.MessageOffset))
			m.Version = entities.DefaultMetricVersion
		}

		switch {
		case !m.Usage.Quantity.IsInt():
			logger.Debug(
				"not int quantity", logf.ErrorKey("usage.quantity"), logf.ErrorValue(m.Usage.Quantity.String()),
				logf.Offset(m.MessageOffset),
			)
			invalid = append(invalid, v.makeIncorrectMetric(
				m, entities.FailedByInvalidModel, fmt.Sprintf("usage.quantity not an integer(%d)", m.Usage.Quantity),
			))
		case m.Usage.Type == entities.UnknownUsage:
			logger.Debug(
				"unknown usage type", logf.ErrorKey("usage.type"), logf.ErrorValue(m.Usage.RawType),
				logf.Offset(m.MessageOffset),
			)
			invalid = append(invalid, v.makeIncorrectMetric(
				m, entities.FailedByInvalidModel, fmt.Sprintf("usage.type unknown(%s)", m.Usage.RawType),
			))
		case m.Usage.Start.IsZero():
			logger.Debug(
				"empty usage start", logf.ErrorKey("usage.start"), logf.Offset(m.MessageOffset),
			)
			invalid = append(invalid, v.makeIncorrectMetric(
				m, entities.FailedByInvalidModel, "usage.start required",
			))
		case m.Usage.Finish.IsZero():
			logger.Debug(
				"empty usage finish", logf.ErrorKey("usage.finish"), logf.Offset(m.MessageOffset),
			)
			invalid = append(invalid, v.makeIncorrectMetric(
				m, entities.FailedByInvalidModel, "usage.finish required",
			))
		case m.Usage.Unit == "":
			logger.Debug(
				"empty usage unit", logf.ErrorKey("usage.unit"), logf.Offset(m.MessageOffset),
			)
			invalid = append(invalid, v.makeIncorrectMetric(
				m, entities.FailedByInvalidModel, "usage.unit required",
			))
		case m.SourceWT.IsZero():
			logger.Debug(
				"empty source_wt", logf.ErrorKey("source_wt"), logf.Offset(m.MessageOffset),
			)
			invalid = append(invalid, v.makeIncorrectMetric(
				m, entities.FailedByInvalidModel, "source_wt required",
			))
		case m.CloudID == "" && m.FolderID == "" && m.AbcID == 0 && m.AbcFolderID == "" && m.BillingAccountID == "":
			logger.Debug("empty metric resources ids", logf.Offset(m.MessageOffset))
			invalid = append(invalid, v.makeIncorrectMetric(
				m, entities.FailedByInvalidModel,
				"One of resource_id, cloud_id, folder_id, abc_id, billing_account_id required",
			))
		default:
			valid = append(valid, m)
		}
	}
	return
}

func (v *metricValuesValidator) validatePositiveQty(
	metrics []entities.SourceMetric,
) (valid []entities.SourceMetric, invalid []entities.InvalidMetric, err error) {
	logger := tooling.Logger(v.ctx)
	valid = make([]entities.SourceMetric, 0, len(metrics))
	for _, m := range metrics {
		switch {
		case !m.Usage.Quantity.IsFinite():
			logger.Debug(
				"incorrect quantity", logf.ErrorKey("usage.quantity"), logf.ErrorValue("NaN"),
				logf.Offset(m.MessageOffset),
			)
			invalid = append(invalid, v.makeIncorrectMetric(
				m, entities.FailedByNegativeQuantity, "usage.quantity is NaN",
			))
		case m.Usage.Quantity.Sign() < 0:
			logger.Debug(
				"incorrect quantity", logf.ErrorKey("usage.quantity"), logf.ErrorValue(m.Usage.Quantity.String()),
				logf.Offset(m.MessageOffset),
			)
			invalid = append(invalid, v.makeIncorrectMetric(
				m, entities.FailedByInvalidModel, fmt.Sprintf("usage.quantity %d is not positive", m.Usage.Quantity),
			))
		default:
			valid = append(valid, m)
		}
	}
	return
}

func (v *metricValuesValidator) validateWriteLag(
	metrics []entities.SourceMetric, metricLifeTime time.Duration,
) (valid []entities.SourceMetric, invalid []entities.InvalidMetric, err error) {
	const allowed = time.Minute * 10

	logger := tooling.Logger(v.ctx)
	valid = make([]entities.SourceMetric, 0, len(metrics))
	for _, m := range metrics {
		switch {
		case m.MessageWriteTime.IsZero():
			logger.Debug(
				"empty message write time", logf.ErrorKey("message_write_time"), logf.Offset(m.MessageOffset),
			)
			invalid = append(invalid, v.makeIncorrectMetric(
				m, entities.FailedByFinishedAfterWrite, "empty message write time",
			))
		case m.Usage.Finish.IsZero():
			logger.Debug(
				"empty usage.finish", logf.ErrorKey("usage.finish"), logf.Offset(m.MessageOffset),
			)
			invalid = append(invalid, v.makeIncorrectMetric(
				m, entities.FailedByFinishedAfterWrite, "empty usage.finish",
			))
		case m.MessageWriteTime.Sub(m.Usage.Finish) < -allowed: // finish > write time + allowed
			logger.Debug(
				"incorrect finish time", logf.ErrorKey("usage.finish"), logf.ErrorValue(m.Usage.Finish.String()),
				logf.Offset(m.MessageOffset),
			)
			invalid = append(invalid, v.makeIncorrectMetric(
				m, entities.FailedByFinishedAfterWrite, fmt.Sprintf(
					"usage.finish greater than write time %d > %d + %d",
					m.Usage.Finish.Unix(), m.MessageWriteTime.Unix(), int(allowed.Seconds()),
				),
			))
		case m.MessageWriteTime.Sub(m.Usage.Finish) > metricLifeTime: // finish < write time - read grace
			logger.Debug(
				"incorrect finish time", logf.ErrorKey("usage.finish"), logf.ErrorValue(m.Usage.Finish.String()),
				logf.Offset(m.MessageOffset),
			)
			invalid = append(invalid, v.makeIncorrectMetric(
				m, entities.FailedByExpired, fmt.Sprintf(
					"usage.finish too much before write time %d < %d - %d",
					m.Usage.Finish.Unix(), m.MessageWriteTime.Unix(), int(metricLifeTime.Seconds()),
				),
			))
		default:
			valid = append(valid, m)
		}
	}
	return
}

func (v *metricValuesValidator) validateWriteGrace(
	metrics []entities.SourceMetric, graceTime time.Time,
) (valid []entities.SourceMetric, invalid []entities.InvalidMetric, err error) {
	logger := tooling.Logger(v.ctx)
	valid = make([]entities.SourceMetric, 0, len(metrics))
	for _, m := range metrics {
		switch {
		case !m.Usage.Finish.After(graceTime):
			logger.Debug(
				"finish time less than grace", logf.ErrorKey("usage.finish"), logf.ErrorValue(m.Usage.Finish.String()),
				logf.Offset(m.MessageOffset),
			)
			invalid = append(invalid, v.makeIncorrectMetric(
				m, entities.FailedByExpired, fmt.Sprintf(
					"usage.finish not after grace %d < %d",
					m.Usage.Finish.Unix(), graceTime.Unix(),
				),
			))
		default:
			valid = append(valid, m)
		}
	}
	return
}

type metricsUniquenessValidator struct {
	validatorCommon

	ctx    context.Context
	scope  entities.ProcessingScope
	seeker DuplicatesSeeker

	logger log.Structured

	periodIDs map[time.Time][]entities.MetricIdentity
	periods   []time.Time

	metricsIndex    map[entities.MetricIdentity]int
	duplicatesIndex map[int]struct{}
}

func (v *metricsUniquenessValidator) validate(
	metrics []entities.SourceMetric,
) (valid []entities.SourceMetric, duplicates []entities.InvalidMetric, err error) {
	for _, mID := range v.buildIDs(metrics) {
		if !v.pushToIndex(mID) {
			v.logDuplicate(mID.id)
			continue
		}
		v.pushToPeriods(mID)
	}

	if err := v.seekDuplicates(); err != nil {
		return nil, nil, err
	}

	if len(v.duplicatesIndex) == 0 {
		return metrics, nil, nil
	}

	// temporary commented
	// dropDuplicates := tooling.Features(v.ctx).DropDuplicates()
	valid = make([]entities.SourceMetric, 0, len(metrics))

	for i, m := range metrics {
		if v.checkDuplicateByPos(i) {
			duplicates = append(duplicates, v.makeIncorrectMetric(
				m, entities.FailedByDuplicate, fmt.Sprintf("'%s::%s' is duplicate", m.Schema, m.MetricID),
			))
			// if dropDuplicates {
			// 	continue
			// }
		}

		// all metrics (even duplicates are valid metrics)
		valid = append(valid, m)
	}
	return valid, duplicates, nil
}

func (v *metricsUniquenessValidator) buildIDs(metrics []entities.SourceMetric) []metricIdentityIndex {
	ids := make([]metricIdentityIndex, len(metrics))
	for i, m := range metrics {
		ids[i] = metricIdentityIndex{
			id: entities.MetricIdentity{
				Schema:   m.Schema,
				MetricID: m.MetricID,
				Offset:   m.Offset(),
			},
			pos:    i,
			period: nextBillMonthStartTime(tooling.LocalTime(v.ctx, m.Usage.UsageTime())),
		}
	}
	sort.Slice(ids, func(i, j int) bool { return ids[i].Less(ids[j]) })
	return ids
}

func (v *metricsUniquenessValidator) pushToIndex(mID metricIdentityIndex) (uniq bool) {
	if v.metricsIndex == nil {
		v.metricsIndex = make(map[entities.MetricIdentity]int)
	}
	if v.duplicatesIndex == nil {
		v.duplicatesIndex = make(map[int]struct{})
	}
	id := mID.id
	id.Offset = 0
	if _, found := v.metricsIndex[id]; found {
		v.duplicatesIndex[mID.pos] = struct{}{}
		return false
	}
	v.metricsIndex[id] = mID.pos
	return true
}

func (v *metricsUniquenessValidator) pushDuplicate(id entities.MetricIdentity) {
	v.logDuplicate(id)
	id.Offset = 0
	pos := v.metricsIndex[id]
	v.duplicatesIndex[pos] = struct{}{}
}

func (v *metricsUniquenessValidator) checkDuplicateByPos(i int) bool {
	_, dup := v.duplicatesIndex[i]
	return dup
}

func (v *metricsUniquenessValidator) pushToPeriods(mID metricIdentityIndex) {
	if v.periodIDs == nil {
		v.periodIDs = make(map[time.Time][]entities.MetricIdentity)
	}
	if _, found := v.periodIDs[mID.period]; !found {
		v.periods = append(v.periods, mID.period)
	}
	v.periodIDs[mID.period] = append(v.periodIDs[mID.period], mID.id)
}

func (v *metricsUniquenessValidator) seekDuplicates() error {
	sort.Slice(v.periods, func(i, j int) bool { return v.periods[i].Before(v.periods[j]) })

	for _, period := range v.periods {
		dups, err := v.seeker.FindDuplicates(v.ctx, v.scope, period, v.periodIDs[period])
		if err != nil {
			return err
		}
		for _, dup := range dups {
			v.pushDuplicate(dup)
		}
	}
	return nil
}

func (v *metricsUniquenessValidator) logDuplicate(id entities.MetricIdentity) {
	if v.logger == nil {
		v.logger = tooling.Logger(v.ctx)
	}
	v.logger.Debug("duplicate metric",
		logf.Offset(id.Offset), logf.ErrorValue(fmt.Sprintf("%s::%s", id.Schema, id.MetricID)),
	)
}

type validatorCommon struct{}

func (validatorCommon) makeIncorrectMetric(
	m entities.SourceMetric, reason entities.MetricFailReason, comment string,
) entities.InvalidMetric {
	im := entities.InvalidMetric{}
	im.Metric = m
	im.SourceWT = m.SourceWT
	im.MessageWriteTime = m.MessageWriteTime
	im.MessageOffset = m.MessageOffset
	im.Reason = reason
	im.ReasonComment = comment
	return im
}

type metricIdentityIndex struct {
	id     entities.MetricIdentity
	pos    int
	period time.Time
}

func (m metricIdentityIndex) Less(o metricIdentityIndex) bool {
	switch {
	case m.id.Cmp(o.id) < 0:
		return true
	case m.period.Before(o.period):
		return true
	}
	return m.pos < o.pos
}
