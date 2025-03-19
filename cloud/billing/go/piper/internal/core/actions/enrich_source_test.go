package actions

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
)

type enrichSourceTestSuite struct {
	suite.Suite
}

func TestEnrichSource(t *testing.T) {
	suite.Run(t, new(enrichSourceTestSuite))
}

func (suite *enrichSourceTestSuite) TestEnrichSource() {
	metric := entities.SourceMetric{}
	metric.Tags = `{}`
	metrics := []entities.SourceMetric{metric}

	enrichedMetrics, err := StartEnrichment(context.Background(), entities.ProcessingScope{}, metrics)

	suite.Require().NoError(err)
	suite.Require().Equal(metric, enrichedMetrics[0].SourceMetric)
}

func (suite *enrichSourceTestSuite) TestEnrichResourceBinding() {
	cases := []struct {
		schema              string
		resourceBindingType entities.ResourceBindingType
	}{
		{"b2b.tracker.license.v1", entities.TrackerResourceBinding},
		{"b2b.tracker.activity.v1", entities.TrackerResourceBinding},
		{"something else", entities.NoResourceBinding},
	}

	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.schema), func() {
			metric := entities.SourceMetric{}
			metric.Schema = c.schema
			metrics := []entities.SourceMetric{metric}

			enrichedMetrics, err := StartEnrichment(context.Background(), entities.ProcessingScope{}, metrics)

			suite.Require().NoError(err)
			suite.Require().Equal(metric, enrichedMetrics[0].SourceMetric)
			suite.Require().Equal(c.resourceBindingType, enrichedMetrics[0].ResourceBindingType)
		})
	}
}

func (suite *enrichSourceTestSuite) TestEnrichProducts() {
	metric := entities.SourceMetric{}
	metric.Tags = `{"product_ids": ["a", "b"]}`
	metrics := []entities.SourceMetric{metric}

	enrichedMetrics, err := StartEnrichment(context.Background(), entities.ProcessingScope{}, metrics)

	suite.Require().NoError(err)
	suite.Require().ElementsMatch([]string{"a", "b"}, enrichedMetrics[0].Products)
}

func (suite *enrichSourceTestSuite) TestMetricPeriod() {
	now := time.Date(2000, 1, 1, 12, 0, 0, 0, time.UTC)

	cases := []struct {
		inp  entities.MetricPeriod
		want entities.MetricPeriod
	}{
		{
			inp:  entities.MetricPeriod{Start: now, Finish: now.Add(time.Hour)},
			want: entities.MetricPeriod{Start: now, Finish: now.Add(time.Hour)},
		},
		{
			inp:  entities.MetricPeriod{Start: now.Add(time.Minute), Finish: now.Add(time.Minute)},
			want: entities.MetricPeriod{Start: now, Finish: now.Add(time.Hour)},
		},
		{
			inp:  entities.MetricPeriod{Start: now.Add(time.Hour), Finish: now.Add(time.Hour)},
			want: entities.MetricPeriod{Start: now.Add(time.Hour), Finish: now.Add(time.Hour)},
		},
		{ // NOTE: Assume that for enrichment we got metrics in one hour
			inp:  entities.MetricPeriod{Start: now.Add(-time.Hour), Finish: now.Add(time.Minute)},
			want: entities.MetricPeriod{Start: now, Finish: now.Add(time.Hour)},
		},
	}

	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			metrics := []entities.SourceMetric{{
				Usage: entities.MetricUsage{
					Start:  c.inp.Start,
					Finish: c.inp.Finish,
				},
			}}

			enrichedMetrics, err := StartEnrichment(context.Background(), entities.ProcessingScope{}, metrics)
			suite.Require().NoError(err)
			suite.EqualValues(c.want, enrichedMetrics[0].Period)
		})
	}
}
