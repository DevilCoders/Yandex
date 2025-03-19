package handlers

import (
	"bytes"
	"encoding/json"
	"github.com/mailru/easyjson/jlexer"
	"io"
	"strings"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/types"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
)

const (
	messages         = 1000
	metricPerMessage = 100
)

var source = func() (result []lbtypes.ReadMessage) {
	metric := types.SourceMetric{}
	metric.ID = "some-id"
	metric.Schema = "metric-schema"
	metric.FolderID = "folder-id"

	metric.Usage = types.MetricUsage{
		Quantity: decimal.Must(decimal.FromInt64(999)),
		Type:     "delta",
		Unit:     "metric unit",
		Start:    types.JSONTimestamp(time.Now()),
		Finish:   types.JSONTimestamp(time.Now()),
	}
	metric.Tags = types.JSONAnything(`{"tag": 123}`)
	metric.Version = "v8"

	buf := bytes.Buffer{}
	enc := json.NewEncoder(&buf)
	for i := 0; i < messages; i++ {
		buf.Reset()
		for j := 0; j < metricPerMessage; j++ {
			_ = enc.Encode(metric)
		}
		result = append(result, lbtypes.ReadMessage{DataReader: lbtypes.NewTestReader(buf.Bytes())})
	}
	return
}()

func BenchmarkParseSourceMetrics(b *testing.B) {
	parser := sourceMetricsParser{}
	for i := 0; i < b.N; i++ {
		p, _ := parser.parseMessages(source)
		if len(p) != metricPerMessage*messages {
			b.Fatalf("incorrect metrics count %d", len(p))
		}
	}
}

func BenchmarkParseOneMetric(b *testing.B) {
	parser := sourceMetricsParser{}
	src := source[0]
	for i := 0; i < b.N; i++ {
		p, _ := parser.parseOneMessage(src.DataReader)
		if len(p.v) != metricPerMessage {
			b.Fatalf("incorrect metrics count %d", len(p.v))
		}
		_ = src.Close()
		smPool.Put(p)
	}
}

// Some services send metrics with `..."labels": [], ...`, those should be parsed correctly
func TestParseMetricWhereLabelsIsEmptyList(t *testing.T) {
	validMessage := `
{"id":"0c236f56-257d-4899-962f-5a353d71c299","schema":"ai.speech.stt.v1","cloud_id":"","folder_id":"b1guf54kfe074cin9d97","labels":[],"tags":{},"usage":{"quantity":2,"start":1644905716,"finish":1644905716,"unit":"15sec","type":"delta"},"source_id":"cl1k486ac51r7a17vlvf-urir","source_wt":1644905716,"resource_id":"","version":"v1alpha1"}
`
	parser := sourceMetricsParser{}
	p, iss := parser.parseOneMessage(strings.NewReader(validMessage))
	require.Len(t, p.v, 1)
	assert.Equal(t, "0c236f56-257d-4899-962f-5a353d71c299", p.v[0].ID)
	require.Len(t, iss, 0)
}

func TestParseValidMetricAfterInvalidOnSameLine(t *testing.T) {
	messageWithValidMetricAfterInvalidOnSameLine := `
{"labels": "invalid"}{"id":"23436f56-257d-4899-962f-12353d71c299","schema":"ai.speech.stt.v1","cloud_id":"","folder_id":"b1guf54kfe074cin9d97","labels":{},"tags":{},"usage":{"quantity":2,"start":1644905716,"finish":1644905716,"unit":"15sec","type":"delta"},"source_id":"cl1k486ac51r7a17vlvf-urir","source_wt":1644905716,"resource_id":"","version":"v1alpha1"}
`
	parser := sourceMetricsParser{}
	p, iss := parser.parseOneMessage(strings.NewReader(messageWithValidMetricAfterInvalidOnSameLine))
	require.Len(t, p.v, 1)
	assert.Equal(t, "23436f56-257d-4899-962f-12353d71c299", p.v[0].ID)
	require.Len(t, iss, 1)
	var lexerErr *jlexer.LexerError
	assert.ErrorAs(t, iss[0].err, &lexerErr)
	assert.Equal(t, []byte(`{"labels": "invalid"}`), iss[0].data)
}

func TestInvalidCharAfterObjKeyErrorHandling(t *testing.T) {
	messageWithInvalidCharAfterObjKey := `{"labels": {"key"some_key}}`
	parser := sourceMetricsParser{}
	p, iss := parser.parseOneMessage(strings.NewReader(messageWithInvalidCharAfterObjKey))
	require.Len(t, p.v, 0)
	require.Len(t, iss, 1)
	var syntErr *json.SyntaxError
	assert.ErrorAs(t, iss[0].err, &syntErr)
	assert.Equal(t, "invalid character 's' after object key", iss[0].err.Error())
	assert.Equal(t, []byte(messageWithInvalidCharAfterObjKey), iss[0].data)
}

func TestParseValidMetricAfterInvalid(t *testing.T) {
	messageWithValidMetricAfterInvalid := `
{"id":"f81643ca-8f8c-46fb-99a9-6e71de44682f","schema":"ai.speech.stt.v1","cloud_id":"","folder_id":"b1g0fiaicjsopef7a9g4","labels":["lbl"],"tags":{},"usage":{"quantity":14,"start":1644905716,"finish":1644905716,"unit":"15sec","type":"delta"},"source_id":"cl1k486ac51r7a17vlvf-urir","source_wt":1644905716,"resource_id":"","version":"v1alpha1"}
{"id":"0c236f56-257d-4899-962f-5a353d71c299","schema":"ai.speech.stt.v1","cloud_id":"","folder_id":"b1guf54kfe074cin9d97","labels":{},"tags":{},"usage":{"quantity":2,"start":1644905716,"finish":1644905716,"unit":"15sec","type":"delta"},"source_id":"cl1k486ac51r7a17vlvf-urir","source_wt":1644905716,"resource_id":"","version":"v1alpha1"}
`
	parser := sourceMetricsParser{}
	p, iss := parser.parseOneMessage(strings.NewReader(messageWithValidMetricAfterInvalid))
	require.Len(t, p.v, 1)
	assert.Equal(t, "0c236f56-257d-4899-962f-5a353d71c299", p.v[0].ID)
	require.Len(t, iss, 1)
	assert.NotNil(t, iss[0].data)
	assert.Error(t, iss[0].err)
	assert.Equal(t, "f81643ca-8f8c-46fb-99a9-6e71de44682f", iss[0].metric.ID)
}

func TestParseInvalidJSON(t *testing.T) {
	invalidMessage := `
{
{"x":[1,2,3,4,5],[}}{}
`
	parser := sourceMetricsParser{}
	p, iss := parser.parseOneMessage(strings.NewReader(invalidMessage))
	require.Len(t, p.v, 0)
	require.Len(t, iss, 2)
	assert.ErrorIs(t, iss[0].err, io.ErrUnexpectedEOF)
	assert.Equal(t, []byte("{"), iss[0].data)
	var syntErr *json.SyntaxError
	assert.ErrorAs(t, iss[1].err, &syntErr)
	assert.Equal(t, []byte(`{"x":[1,2,3,4,5],[}}{}`), iss[1].data)
}
