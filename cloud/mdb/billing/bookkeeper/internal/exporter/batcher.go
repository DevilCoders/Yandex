package exporter

import (
	"bytes"

	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/exporter/model"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
)

type MetricsBatcher interface {
	ID() string
	Add(m model.Metric)
	Marshal() ([]byte, error)
	Reset() error
	Empty() bool
}

type YCMetricsBatcher struct {
	metrics []model.Metric
	buf     *bytes.Buffer
	idgen   generator.IDGenerator
	id      string
	done    bool
}

func NewYCMetricsBatcher(idgen generator.IDGenerator) (*YCMetricsBatcher, error) {
	id, err := idgen.Generate()
	if err != nil {
		return nil, err
	}
	return &YCMetricsBatcher{buf: &bytes.Buffer{}, id: id, idgen: idgen}, nil
}

func (mb *YCMetricsBatcher) ID() string {
	return mb.id
}

func (mb *YCMetricsBatcher) Add(m model.Metric) {
	mb.metrics = append(mb.metrics, m)
}

func (mb *YCMetricsBatcher) Marshal() ([]byte, error) {
	defer mb.buf.Reset()
	for i := range mb.metrics {
		mbytes, err := mb.metrics[i].Marshal()
		if err != nil {
			return nil, err
		}

		if _, err := mb.buf.Write(mbytes); err != nil {
			return nil, err
		}

		if err := mb.buf.WriteByte('\n'); err != nil {
			return nil, err
		}
	}
	return mb.buf.Bytes(), nil
}

func (mb *YCMetricsBatcher) Reset() error {
	mb.metrics = make([]model.Metric, 0)
	mb.buf.Reset()
	id, err := mb.idgen.Generate()
	if err != nil {
		return err
	}
	mb.id = id
	return nil
}

func (mb *YCMetricsBatcher) Empty() bool {
	return len(mb.metrics) == 0
}
