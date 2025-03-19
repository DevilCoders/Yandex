package handlers

import (
	"fmt"
	"os"
	"strings"
	"testing"
	"time"

	"github.com/jonboulle/clockwork"
	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/servers/logbroker/handlers/mocks"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/types"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
)

func TestMain(m *testing.M) {
	hostname = "test-host"
	exitCode := m.Run()

	os.Exit(exitCode)
}

func intDec(i int) decimal.Decimal128 {
	return decimal.Must(decimal.FromInt64(int64(i)))
}

func strDec(s string) decimal.Decimal128 {
	return decimal.Must(decimal.FromString(s))
}

func getLastMockCall(method string, calls []mock.Call) (result mock.Call) {
	for _, c := range calls {
		if c.Method == method {
			result = c
		}
	}
	return result
}

func getClock() clockwork.FakeClock {
	return clockwork.NewFakeClockAt(time.Date(2000, 1, 1, 0, 0, 0, 0, time.UTC))
}

type messagesMocks struct {
	messages lbtypes.Messages
	reporter mocks.MessagesReporter

	metricNo int
}

func (m *messagesMocks) SetupTest() {
	m.reporter = mocks.MessagesReporter{}
	m.resetMessages()
}

func (m *messagesMocks) pushMessage(data ...string) {
	m.messages.LastOffset++
	for _, d := range data {
		d := strings.ReplaceAll(d, "\n", " ")
		m.messages.Messages = append(m.messages.Messages, lbtypes.ReadMessage{
			Offset:     m.messages.LastOffset,
			SeqNo:      m.messages.LastOffset + 1,
			SourceID:   []byte("test-source"),
			CreateTime: m.messages.LastWriteTime,
			WriteTime:  m.messages.LastWriteTime,
			DataReader: lbtypes.NewTestReader([]byte(d)),
		})
	}
}

func (m *messagesMocks) resetMessages() {
	m.messages = lbtypes.Messages{
		MessagesReporter: &m.reporter,
		LastWriteTime:    getClock().Now(),
	}
	m.metricNo = 0
}

func (m *messagesMocks) formatSourceMessage(schema string, from, to time.Time, quantity int, tags string) string {
	defer func() { m.metricNo++ }()
	message := `{
		"source_id": "logbroker-grpc:topic:partition",
		"raw_metric": null,
		"resource_id": "resource_id",
		"schema": "%s",
		"billing_account_id": "ba",
		"usage": {
			"start": %d,
			"finish": %d,
		    "quantity": %d
		},
	    "tags": %s,
		"id": "77d5583c-4a52-41c7-8f41-72abb4f142f4:%d"
	}`

	return fmt.Sprintf(message, schema, from.Unix(), to.Unix(), quantity, tags, m.metricNo)
}

type metricsMaker struct{}

func (m metricsMaker) makeCommonEnriched(
	schema string, metricNo int, quantity decimal.Decimal128, start, finish, messageWriteTime time.Time,
	usageType entities.UsageType, tags string,
) entities.EnrichedMetric {
	return entities.EnrichedMetric{
		SourceMetric: entities.SourceMetric{
			MetricID:         fmt.Sprintf("77d5583c-4a52-41c7-8f41-72abb4f142f4:%d", metricNo),
			ResourceID:       "resource_id",
			Schema:           schema,
			BillingAccountID: "ba",
			SkuID:            "sku_id",
			Usage: entities.MetricUsage{
				Quantity: quantity,
				Start:    start,
				Finish:   finish,
			},
			SourceID: "logbroker-grpc:topic:partition",
			Labels: entities.Labels{
				User: map[string]string{},
			},
			Tags:             types.JSONAnything(tags),
			MessageWriteTime: messageWriteTime,
			MessageOffset:    1,
		},
		Period: entities.MetricPeriod{
			Start:  start.Truncate(time.Hour).Local(),
			Finish: finish.Truncate(time.Hour).Add(time.Hour).Local(),
		},
		SkuInfo: entities.SkuInfo{
			SkuUsageType: usageType,
			PricingUnit:  "pricing_unit",
		},
		PricingQuantity:     quantity,
		Products:            nil,
		ResourceBindingType: 0,
		TagsOverride:        nil,
		MasterAccountID:     "",
		ReshardingKey:       "ba",
	}
}
