package handlers

import (
	"bufio"
	"bytes"
	"encoding/json"
	"errors"
	"io"
	"sync"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/types"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
)

func readMessagesToInvalidMetrics(msg []lbtypes.ReadMessage) (
	invalid []entities.InvalidMetric,
) {
	for _, m := range msg {
		data, _ := io.ReadAll(m)
		invalid = append(invalid,
			entities.InvalidMetric{
				IncorrectRawMessage: entities.IncorrectRawMessage{
					Reason:           entities.FailedByTooBigChunk,
					ReasonComment:    "",
					RawMetric:        data,
					MessageWriteTime: m.WriteTime,
					MessageOffset:    m.Offset,
				},
			},
		)
	}
	return
}

type sourceMetricsParser struct{}

func (p sourceMetricsParser) parseMessages(msg []lbtypes.ReadMessage) (
	parsed []entities.SourceMetric, invalid []entities.InvalidMetric,
) {
	metricsParsed := make([]*smSlice, len(msg))

	{
		// Some kind of optimization - we parse incoming messages in parallel workers for some performance improvement.
		// This code is typically has controlled parallel executions and depends on number of incoming readers,
		// so parallel parameters hardcoded.

		var im sync.Mutex
		var wg sync.WaitGroup
		wrk := make(chan struct{}, 32)

		wg.Add(len(msg))
		for i := range msg {
			go func(i int) {
				defer wg.Done()

				// NOTE: simple semaphore
				wrk <- struct{}{}
				defer func() {
					<-wrk
					_ = msg[i].Close()
				}()

				m := msg[i]
				parsedMessage, issues := p.parseOneMessage(m.DataReader)
				metricsParsed[i] = parsedMessage
				if len(issues) == 0 {
					return
				}
				im.Lock()
				defer im.Unlock()
				for _, issue := range issues {
					var metric entities.Metric
					if issue.metric.Schema != "" {
						metric = p.sourceToEnt(issue.metric)
					}
					invalid = append(invalid,
						entities.InvalidMetric{
							Metric: metric,
							IncorrectRawMessage: entities.IncorrectRawMessage{
								Reason:           entities.FailedByUnparsedJSON,
								ReasonComment:    issue.err.Error(),
								RawMetric:        issue.data,
								MessageWriteTime: m.WriteTime,
								MessageOffset:    m.Offset,
							},
						},
					)
				}
			}(i)
		}
		wg.Wait()
	}

	metricsCount := 0
	for _, mp := range metricsParsed {
		if mp != nil {
			metricsCount += len(mp.v)
		}
	}

	parsed = make([]entities.SourceMetric, 0, metricsCount)
	for i, mp := range metricsParsed {
		if mp != nil {
			parsed = p.makeMetrics(parsed, msg[i], mp.v)
			smPool.Put(mp)
		}
	}

	return
}

type smSlice struct {
	v []types.SourceMetric
}

type smParseError struct {
	data   []byte
	err    error
	metric types.SourceMetric
}

var smPool = sync.Pool{New: func() interface{} { return &smSlice{v: make([]types.SourceMetric, 0, 100)} }}

func (sourceMetricsParser) parseOneMessage(
	r io.Reader,
) (parsed *smSlice, issues []smParseError) {
	scn := bufio.NewScanner(r)
	reader := bytes.Reader{}
	parsed = smPool.Get().(*smSlice)
	parsed.v = parsed.v[:0]

	for scn.Scan() {
		line := scn.Bytes()
		reader.Reset(line)
		dec := json.NewDecoder(&reader)
		dec.DisallowUnknownFields()

		sm := types.SourceMetrics{}
		var startPos int64 = -1
		for dec.More() {
			if startPos == dec.InputOffset() { // catch infinite loop
				panic("no parsing position advance")
			}
			startPos = dec.InputOffset()
			if err := dec.Decode(&sm); err != nil {
				iss := smParseError{
					err: err,
				}

				var syntErr *json.SyntaxError
				canContinue := false
				switch {
				case errors.As(err, &syntErr),
					errors.Is(err, io.ErrUnexpectedEOF):
					iss.data = append([]byte(nil), line[startPos:]...)
				default:
					iss.data = append([]byte(nil), line[startPos:dec.InputOffset()]...)
					canContinue = true
				}

				if len(sm) == 1 { // if no workaround applied
					iss.metric = sm[0]
				}
				issues = append(issues, iss)
				sm = sm[:0]
				if canContinue {
					continue
				}
				break
			}
			parsed.v = append(parsed.v, sm...)
			sm = sm[:0]
		}
	}

	return
}

func (p sourceMetricsParser) makeMetrics(
	appendTo []entities.SourceMetric, m lbtypes.ReadMessage, parsed []types.SourceMetric,
) []entities.SourceMetric {
	if len(parsed) == 0 {
		return appendTo
	}

	for _, pm := range parsed {
		ent := p.sourceToEnt(pm)
		ent.MessageWriteTime = m.WriteTime
		ent.MessageOffset = m.Offset

		appendTo = append(appendTo, ent)
	}
	return appendTo
}

func (sourceMetricsParser) sourceToEnt(src types.SourceMetric) entities.SourceMetric {
	return entities.SourceMetric{
		MetricID: notEmptyID(src.ID),
		Schema:   src.Schema,
		Version:  src.Version,

		CloudID:     src.CloudID,
		FolderID:    src.FolderID,
		AbcID:       src.AbcID,
		AbcFolderID: src.AbcFolderID,

		ResourceID:       src.ResourceID,
		BillingAccountID: src.BillingAccountID,

		SkuID: src.SkuID,

		Labels: jsonSourceLabelsToEntity(src.UserLabels),
		Tags:   src.Tags,

		Usage: entities.MetricUsage{
			Quantity: src.Usage.Quantity,
			Start:    src.Usage.Start.Time(),
			Finish:   src.Usage.Finish.Time(),
			Unit:     src.Usage.Unit,
			RawType:  src.Usage.Type,
			Type:     jsonUsageTypeToEntity(src.Usage.Type),
		},
		SourceID: src.SourceID,
		SourceWT: src.SourceWriteTime.Time(),
	}
}

func jsonSourceLabelsToEntity(l map[string]types.JSONAnyString) (r entities.Labels) {
	r.User = make(map[string]string)
	for k := range l {
		r.User[k] = l[k].String()
	}
	return
}

func jsonUsageTypeToEntity(t string) entities.UsageType {
	switch t {
	case "cumulative":
		return entities.CumulativeUsage
	case "delta", "":
		return entities.DeltaUsage
	case "gauge":
		return entities.GaugeUsage
	}
	return entities.UnknownUsage
}

func notEmptyID(id string) string {
	if id != "" {
		return id
	}
	return uuid.Must(uuid.NewV4()).String()
}
