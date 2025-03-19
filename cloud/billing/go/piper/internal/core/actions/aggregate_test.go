package actions

import (
	"context"
	"sort"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/types"
)

type aggregateTestSuite struct {
	suite.Suite
}

func TestAggregateMetrics(t *testing.T) {
	suite.Run(t, new(aggregateTestSuite))
}

// TestAggregateUnknownTagsFields tests the case when some fields in tags we don't know, so we can not do aggregation
func (suite *aggregateTestSuite) TestAggregateUnknownTagsFields() {
	var tests = []struct {
		schema string
		tags   []string
	}{
		// S3 Metrics
		{
			"s3.api.v1",
			[]string{
				`{"unknown_field": "A","storage_class":"cold","method":"PUT","status_code":"200","net_type":"ingress","transferred":5}`,
				`{"unknown_field": "B", "storage_class":"cold","method":"PUT","status_code":"200","net_type":"ingress","transferred":10}`,
			},
		},
		// DNS Metrics
		{
			"dns.requests.v1",
			[]string{
				`{"unknown_field": "A","request_type":"recursive", "forwarding": true}`,
				`{"unknown_field": "B","request_type":"recursive", "forwarding": true}`,
			},
		},
	}

	for i := range tests {
		suite.Run(tests[i].schema, func() {

			quantity1 := toDec(1)
			quantity2 := toDec(2)

			metric1 := entities.SourceMetric{}
			metric1.Schema = tests[i].schema
			metric1.CloudID = "cloud_id"
			metric1.Usage.Quantity = quantity1
			metric1.Usage.Start = time.Unix(1, 0)
			metric1.Usage.Finish = time.Unix(2, 0)
			metric1.Tags = types.JSONAnything(tests[i].tags[0])

			metric2 := entities.SourceMetric{}
			metric2.Schema = tests[i].schema
			metric2.CloudID = "cloud_id"
			metric2.Usage.Quantity = quantity2
			metric2.Usage.Start = time.Unix(2, 0)
			metric2.Usage.Finish = time.Unix(3, 0)
			metric2.Tags = types.JSONAnything(tests[i].tags[1])

			metrics := []entities.SourceMetric{metric2, metric1}

			result := Aggregate(context.Background(), metrics)

			sort.Slice(result, func(i, j int) bool {
				return result[i].Usage.Finish.Before(result[j].Usage.Finish)
			})

			suite.Require().Len(result, 2)

			suite.Require().Equal(metric1.Tags, result[0].Tags)
			suite.Require().Equal(metric2.Tags, result[1].Tags)

			suite.Require().Equal(metric1.Usage.Quantity, result[0].Usage.Quantity)
			suite.Require().Equal(metric2.Usage.Quantity, result[1].Usage.Quantity)

			suite.Require().Equal(metric1.Usage.Finish, result[0].Usage.Finish)
			suite.Require().Equal(metric2.Usage.Finish, result[1].Usage.Finish)
		})
	}
}

func (suite *aggregateTestSuite) TestAggregate() {

	var tests = []struct {
		schema       string
		tags         []string
		expectedTags string
	}{
		// Aggregate same S3 Metrics
		{
			"s3.api.v1",
			[]string{
				`{"storage_class":"cold", "handler": "PUT", "method":"PUT","status_code":"200","net_type":"ingress","transferred":5}`,
				`{"storage_class":"cold", "handler": "PUT", "method":"PUT","status_code":"200","net_type":"ingress","transferred":10}`,
			},
			`{"handler":"PUT","storage_class":"cold","method":"PUT","status_code":"200","net_type":"ingress","transferred":15}`,
		},
		// Aggregate same DNS Metrics
		{
			"dns.requests.v1",
			[]string{
				`{"request_type":"recursive", "forwarding": true}`,
				`{"request_type":"recursive", "forwarding": true}`,
			},
			`{"request_type":"recursive", "forwarding": true}`,
		},
	}

	for i := range tests {
		suite.Run(tests[i].schema, func() {
			quantity1 := toDec(10)
			quantity2 := toDec(5)

			metric1 := entities.SourceMetric{}
			metric1.Schema = tests[i].schema
			metric1.CloudID = "cloud_id"
			metric1.Usage.Quantity = quantity1
			metric1.Usage.Start = time.Unix(100, 0)
			metric1.Usage.Finish = time.Unix(500, 0)
			metric1.Tags = types.JSONAnything(tests[i].tags[0])

			metric2 := entities.SourceMetric{}
			metric2.Schema = tests[i].schema
			metric2.CloudID = "cloud_id"
			metric2.Usage.Quantity = quantity2
			metric2.Usage.Start = time.Unix(300, 0)
			metric2.Usage.Finish = time.Unix(400, 0)
			metric2.Tags = types.JSONAnything(tests[i].tags[1])
			metrics := []entities.SourceMetric{metric2, metric1}
			valid := Aggregate(context.Background(), metrics)

			expectedTags := tests[i].expectedTags
			suite.Require().Len(valid, 1)
			suite.Require().Equal(types.JSONAnything(expectedTags), valid[0].Tags)

			expectedQuantity := quantity1.Add(quantity2)
			suite.Require().Equal(expectedQuantity, valid[0].Usage.Quantity)

			suite.Require().Equal(valid[0].Usage.Start, valid[0].Usage.Finish)
			suite.Require().Equal(metric1.Usage.Finish, valid[0].Usage.Finish)
		})
	}

}

func (suite *aggregateTestSuite) TestAggregateDifferentCloudKey() {
	var tests = []struct {
		schema string
		tag    string
	}{
		// Aggregate S3 Metrics
		{
			"s3.api.v1",
			`{"storage_class":"cold","method":"GET","status_code":"200","net_type":"ingress","transferred":2}`,
		},
		// Aggregate DNS Metrics
		{
			"dns.requests.v1",
			`{"request_type":"direct", "forwarding": true}`,
		},
	}

	quantity1 := toDec(1)
	quantity2 := toDec(2)

	for i := range tests {
		suite.Run(tests[i].schema, func() {

			metric1 := entities.SourceMetric{}
			metric1.CloudID = "cloud_id_a"
			metric1.Schema = tests[i].schema
			metric1.Usage.Finish = time.Unix(1, 0)
			metric1.Usage.Quantity = quantity1
			metric1.Tags = types.JSONAnything(tests[i].tag)

			metric2 := entities.SourceMetric{}
			metric2.CloudID = "cloud_id_b"
			metric2.Schema = tests[i].schema
			metric2.Usage.Finish = time.Unix(2, 0)
			metric2.Usage.Quantity = quantity2
			metric2.Tags = types.JSONAnything(tests[i].tag)

			metrics := []entities.SourceMetric{metric1, metric2}

			valid := Aggregate(context.Background(), metrics)

			suite.Require().Len(valid, 2)

			sort.Slice(valid, func(i, j int) bool {
				return valid[i].Usage.Finish.Before(valid[j].Usage.Finish)
			})

			suite.Require().Equal(quantity1, valid[0].Usage.Quantity)
			suite.Require().Equal(metric1.Tags, valid[0].Tags)
			suite.Require().Equal(metric1.MessageOffset, valid[0].MessageOffset)

			suite.Require().Equal(quantity2, valid[1].Usage.Quantity)
			suite.Require().Equal(metric2.Tags, valid[1].Tags)
			suite.Require().Equal(metric2.MessageOffset, valid[1].MessageOffset)
		})
	}
}

func (suite *aggregateTestSuite) TestAggregateDifferentTagsKey() {

	var tests = []struct {
		schema       string
		tags         []string
		expectedTags []string
	}{
		// Aggregate S3 Metrics
		{
			"s3.api.v1",
			[]string{
				// method PUT
				`{"storage_class":"cold","method":"PUT","status_code":"200","net_type":"ingress","transferred":10}`,
				// method GET
				`{"storage_class":"cold","method":"GET","status_code":"200","net_type":"ingress","transferred":5}`,
				`{"storage_class":"cold","method":"GET","status_code":"200","net_type":"ingress","transferred":2}`,
			},
			[]string{
				`{"storage_class":"cold","method":"PUT","status_code":"200","net_type":"ingress","transferred":10}`,
				`{"storage_class":"cold","method":"GET","status_code":"200","net_type":"ingress","transferred":7}`,
			},
		},
		// Aggregate DNS Metrics
		{
			"dns.requests.v1",
			[]string{
				//  request type `recursive`
				`{"request_type":"recursive", "forwarding": true}`,
				//  request type `direct`
				`{"request_type":"direct", "forwarding": true}`,
				`{"request_type":"direct", "forwarding": true}`,
			}, []string{
				`{"request_type":"recursive", "forwarding": true}`,
				`{"request_type":"direct", "forwarding": true}`,
			},
		},
	}

	quantity1 := toDec(10)
	quantity2 := toDec(5)
	quantity3 := toDec(2)

	for i := range tests {
		suite.Run(tests[i].schema, func() {

			metric1 := entities.SourceMetric{}
			metric1.CloudID = "cloud_id"
			metric1.Schema = tests[i].schema
			metric1.Usage.Finish = time.Unix(1, 0)
			metric1.Usage.Quantity = quantity1
			metric1.Tags = types.JSONAnything(tests[i].tags[0])

			metric2 := entities.SourceMetric{}
			metric2.CloudID = "cloud_id"
			metric2.Schema = tests[i].schema
			metric2.Usage.Finish = time.Unix(2, 0)
			metric2.Usage.Quantity = quantity2
			metric2.Tags = types.JSONAnything(tests[i].tags[1])

			metric3 := entities.SourceMetric{}
			metric3.CloudID = "cloud_id"
			metric3.Schema = tests[i].schema
			metric3.Usage.Finish = time.Unix(3, 0)
			metric3.Usage.Quantity = quantity3
			metric3.Tags = types.JSONAnything(tests[i].tags[2])

			metrics := []entities.SourceMetric{metric1, metric2, metric3}

			valid := Aggregate(context.Background(), metrics)

			suite.Require().Len(valid, 2)

			sort.Slice(valid, func(i, j int) bool {
				return valid[i].Usage.Finish.Before(valid[j].Usage.Finish)
			})

			suite.Require().Equal(quantity1, valid[0].Usage.Quantity)
			suite.Require().Equal(quantity2.Add(quantity3), valid[1].Usage.Quantity)

			expectedTags1 := tests[i].expectedTags[0]
			suite.Require().Equal(types.JSONAnything(expectedTags1), valid[0].Tags)
			expectedTags2 := tests[i].expectedTags[1]
			suite.Require().Equal(types.JSONAnything(expectedTags2), valid[1].Tags)
		})
	}
}

func (suite *aggregateTestSuite) TestStorageAggregateDifferentOffset() {
	var tests = []struct {
		schema string
		tag    string
	}{
		// Aggregate S3 Metrics
		{
			"s3.api.v1",
			`{"storage_class":"cold","method":"GET","status_code":"200","net_type":"ingress","transferred":2}`,
		},
		// Aggregate DNS Metrics
		{
			"dns.requests.v1",
			`{"request_type":"direct", "forwarding": true}`,
		},
	}

	quantity1 := toDec(1)
	quantity2 := toDec(2)

	for i := range tests {
		suite.Run(tests[i].schema, func() {
			metric1 := entities.SourceMetric{}
			metric1.CloudID = "cloud_id"
			metric1.Schema = tests[i].schema
			metric1.Usage.Finish = time.Unix(1, 0)
			metric1.Usage.Quantity = quantity1
			metric1.Tags = types.JSONAnything(tests[i].tag)
			metric1.MessageOffset = 99

			metric2 := entities.SourceMetric{}
			metric2.CloudID = "cloud_id"
			metric2.Schema = tests[i].schema
			metric2.Usage.Finish = time.Unix(2, 0)
			metric2.Usage.Quantity = quantity2
			metric2.Tags = types.JSONAnything(tests[i].tag)
			metric2.MessageOffset = 100

			metrics := []entities.SourceMetric{metric1, metric2}

			valid := Aggregate(context.Background(), metrics)

			suite.Require().Len(valid, 2)

			sort.Slice(valid, func(i, j int) bool {
				return valid[i].Usage.Finish.Before(valid[j].Usage.Finish)
			})

			suite.Require().Equal(quantity1, valid[0].Usage.Quantity)
			suite.Require().Equal(metric1.Tags, valid[0].Tags)
			suite.Require().Equal(metric1.MessageOffset, valid[0].MessageOffset)

			suite.Require().Equal(quantity2, valid[1].Usage.Quantity)
			suite.Require().Equal(metric2.Tags, valid[1].Tags)
			suite.Require().Equal(metric2.MessageOffset, valid[1].MessageOffset)
		})
	}
}
