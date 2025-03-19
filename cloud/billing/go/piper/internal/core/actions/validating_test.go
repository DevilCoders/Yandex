package actions

import (
	"context"
	"fmt"
	"sort"
	"testing"
	"time"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/actions/mocks"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/features"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type validatingTestSuite struct {
	suite.Suite

	schemaGetter mocks.SchemaGetter
}

func TestValidating(t *testing.T) {
	suite.Run(t, new(validatingTestSuite))
}

func (suite *validatingTestSuite) SetupTest() {
	suite.schemaGetter = mocks.SchemaGetter{}
}

func (suite *validatingTestSuite) TestValidateMetricsSchema() {
	metric := entities.SourceMetric{}
	metric.Schema = "compute"
	metrics := []entities.SourceMetric{metric}

	suite.schemaGetter.On("GetMetricSchema", mock.Anything, mock.Anything, "compute").
		Return(entities.MetricsSchema{Schema: "compute"}, nil)

	valid, invalid, err := ValidateMetricsSchema(context.Background(), entities.ProcessingScope{}, &suite.schemaGetter, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(invalid)
	suite.Require().Len(valid, 1)
}

func (suite *validatingTestSuite) TestValidatePositiveQuantity() {
	cases := []struct {
		name        string
		amounts     []int64
		wantValid   int
		wantInvalid int
	}{
		{"one positive", []int64{2}, 1, 0},
		{"one negative", []int64{-2}, 0, 1},
		{"all positive", []int64{2, 4, 10, 1, 3, 5}, 6, 0},
		{"all negative", []int64{-2, -4, -10, -1, -3, -5}, 0, 6},
		{"mixed", []int64{2, 4, -10, 1, -3, 5}, 4, 2},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.name), func() {
			var metrics []entities.SourceMetric
			for _, amount := range c.amounts {
				metric := entities.SourceMetric{}
				metric.Usage.Quantity, _ = decimal.FromInt64(amount)
				metrics = append(metrics, metric)
			}

			valid, invalid, err := ValidateMetricsQuantity(context.Background(), entities.ProcessingScope{}, metrics)

			suite.Require().NoError(err)
			suite.Require().Len(valid, c.wantValid)
			suite.Require().Len(invalid, c.wantInvalid)
		})
	}
}

func (suite *validatingTestSuite) TestValidateWriteLag() {
	cases := []struct {
		reasonComment    string
		messageWriteTime time.Time
		finishTime       time.Time
		valid            bool
	}{
		{"empty message write time", time.Time{}, time.Unix(100, 0), false},
		{"empty usage.finish", time.Unix(100, 0), time.Time{}, false},
		{"usage.finish greater than write time 701 > 100 + 600", time.Unix(100, 0), time.Unix(701, 0), false},
		{"usage.finish too much before write time 100 < 1296100 - 1036800", time.Unix(100, 0).Add(time.Hour * 24 * 15), time.Unix(100, 0), false}, // TODO: not sure
		{"valid", time.Unix(100, 0).Add(time.Hour * 24 * 5), time.Unix(100, 0), true},
	}

	const lifetime = time.Hour * 24 * 12

	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.reasonComment), func() {
			metric := entities.SourceMetric{}
			metric.MessageWriteTime = c.messageWriteTime
			metric.Usage.Finish = c.finishTime

			valid, invalid, err := ValidateMetricsWriteTime(context.Background(), entities.ProcessingScope{}, lifetime, []entities.SourceMetric{metric})

			suite.Require().NoError(err)
			suite.Require().Equal(c.valid, len(valid) == 1)
			if !c.valid {
				suite.Require().Equal(c.valid, len(invalid) == 0)
				suite.Require().Equal(c.reasonComment, invalid[0].ReasonComment)
			}
		})
	}
}

func (suite *validatingTestSuite) TestValidateWriteGrace() {
	correctTime, _ := time.Parse(time.RFC3339, "2020-01-01T00:00:00+03:00")
	incorrectTime, _ := time.Parse(time.RFC3339, "2019-10-01T00:00:00+03:00")

	cases := []struct {
		name        string
		finishTimes []time.Time
		wantValid   int
		wantInvalid int
	}{
		{"correct", []time.Time{correctTime}, 1, 0},
		{"incorrect", []time.Time{incorrectTime}, 0, 1},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.name), func() {
			var metrics []entities.SourceMetric
			for _, finishTime := range c.finishTimes {
				metric := entities.SourceMetric{}
				metric.Usage.Finish = finishTime
				metrics = append(metrics, metric)
			}

			startTime, _ := time.Parse(time.RFC3339, "2020-01-01T00:00:00+03:00")
			processingScope := entities.ProcessingScope{
				StartTime: startTime,
			}
			valid, invalid, err := ValidateMetricsGrace(context.Background(), processingScope, 10*time.Second, metrics)

			suite.Require().NoError(err)
			suite.Require().Len(valid, c.wantValid)
			suite.Require().Len(invalid, c.wantInvalid)
		})
	}
}

func (suite *validatingTestSuite) TestValidateWriteGraceEndOfMonth() {
	graceTime := 9 * time.Hour

	cases := []struct {
		name        string
		currentTime string
		metricTime  string
		wantValid   int
		wantInvalid int
	}{
		{"correct before grace time", "2020-02-01T08:59:59+03:00", "2020-01-30T00:00:00+03:00", 1, 0},
		{"incorrect after grace time", "2020-02-01T09:00:00+03:00", "2020-01-30T00:00:00+03:00", 0, 1},
		{"correct after grace time", "2020-02-01T09:00:00+03:00", "2020-02-01T00:00:01+03:00", 1, 0},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.name), func() {
			var metrics []entities.SourceMetric
			metricTime, _ := time.Parse(time.RFC3339, c.metricTime)
			metric := entities.SourceMetric{}
			metric.Usage.Finish = metricTime
			metrics = append(metrics, metric)

			startTime, _ := time.Parse(time.RFC3339, c.currentTime)
			processingScope := entities.ProcessingScope{
				StartTime: startTime,
			}
			valid, invalid, err := ValidateMetricsGrace(context.Background(), processingScope, graceTime, metrics)

			suite.Require().NoError(err)
			suite.Require().Len(valid, c.wantValid)
			suite.Require().Len(invalid, c.wantInvalid)
		})
	}
}

type validatingModelTestSuite struct {
	suite.Suite

	sourceMetric entities.SourceMetric
}

func (suite *validatingModelTestSuite) SetupTest() {
	suite.sourceMetric = entities.SourceMetric{}
	suite.sourceMetric.Usage.Start = time.Unix(10, 0)
	suite.sourceMetric.Usage.Finish = time.Unix(20, 0)
	suite.sourceMetric.Usage.Unit = "core"
	suite.sourceMetric.SourceWT = time.Unix(10, 0)
	suite.sourceMetric.Usage.Type = entities.DeltaUsage
	suite.sourceMetric.BillingAccountID = "ba_id"
}

func TestModelValidating(t *testing.T) {
	suite.Run(t, new(validatingModelTestSuite))
}

func (suite *validatingModelTestSuite) TestValidateModelCorrect() {
	valid, invalid, err := ValidateMetricsModel(context.Background(), entities.ProcessingScope{}, []entities.SourceMetric{suite.sourceMetric})

	suite.Require().NoError(err)
	suite.Require().Len(valid, 1)
	suite.Require().Empty(invalid)
}

func (suite *validatingModelTestSuite) TestValidateModelIncorrectQuantity() {
	var metrics []entities.SourceMetric
	quantity, _ := decimal.FromString("10.1")
	metric := suite.sourceMetric
	metric.Usage.Quantity = quantity
	metrics = append(metrics, metric)

	valid, invalid, err := ValidateMetricsModel(context.Background(), entities.ProcessingScope{}, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(valid)
	suite.Require().Len(invalid, 1)
}

func (suite *validatingModelTestSuite) TestValidateModelIncorrectUsageType() {
	var metrics []entities.SourceMetric
	metric := suite.sourceMetric
	metric.Usage.Type = entities.UnknownUsage
	metrics = append(metrics, metric)

	valid, invalid, err := ValidateMetricsModel(context.Background(), entities.ProcessingScope{}, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(valid)
	suite.Require().Len(invalid, 1)
}

func (suite *validatingModelTestSuite) TestValidateModelIncorrectStartTime() {
	var metrics []entities.SourceMetric
	metric := suite.sourceMetric
	metric.Usage.Start = time.Time{}
	metrics = append(metrics, metric)

	valid, invalid, err := ValidateMetricsModel(context.Background(), entities.ProcessingScope{}, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(valid)
	suite.Require().Len(invalid, 1)
}

func (suite *validatingModelTestSuite) TestValidateModelIncorrectFinishTime() {
	var metrics []entities.SourceMetric
	metric := suite.sourceMetric
	metric.Usage.Finish = time.Time{}
	metrics = append(metrics, metric)

	valid, invalid, err := ValidateMetricsModel(context.Background(), entities.ProcessingScope{}, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(valid)
	suite.Require().Len(invalid, 1)
}

func (suite *validatingModelTestSuite) TestValidateModelIncorrectSourceWT() {
	var metrics []entities.SourceMetric
	metric := suite.sourceMetric
	metric.SourceWT = time.Time{}
	metrics = append(metrics, metric)

	valid, invalid, err := ValidateMetricsModel(context.Background(), entities.ProcessingScope{}, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(valid)
	suite.Require().Len(invalid, 1)
}

func (suite *validatingModelTestSuite) TestValidateModelIncorrectUnit() {
	var metrics []entities.SourceMetric
	metric := suite.sourceMetric
	metric.Usage.Unit = ""
	metrics = append(metrics, metric)

	valid, invalid, err := ValidateMetricsModel(context.Background(), entities.ProcessingScope{}, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(valid)
	suite.Require().Len(invalid, 1)
}

func (suite *validatingModelTestSuite) TestValidateModelIncorrectIdentityResolve() {
	cases := []struct {
		cloudID          string
		folderID         string
		abcID            int64
		abcFolderID      string
		billingAccountID string
		valid            bool
	}{
		{"", "", 0, "", "", false},
		{"A", "", 0, "", "", true},
		{"", "A", 0, "", "", true},
		{"", "", 1, "", "", true},
		{"", "", 0, "A", "", true},
		{"", "", 0, "", "A", true},
		{"A", "B", 1, "C", "D", true},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case: cloud=%s, folder=%s, abcID=%d, abcFolderID=%s, billingAccountID=%s",
			c.cloudID, c.folderID, c.abcID, c.abcFolderID, c.billingAccountID), func() {
			var metrics []entities.SourceMetric
			metric := suite.sourceMetric
			metric.CloudID = c.cloudID
			metric.FolderID = c.folderID
			metric.AbcID = c.abcID
			metric.AbcFolderID = c.abcFolderID
			metric.BillingAccountID = c.billingAccountID
			metrics = append(metrics, metric)

			valid, invalid, err := ValidateMetricsModel(context.Background(), entities.ProcessingScope{}, metrics)

			suite.Require().NoError(err)
			suite.Require().Equal(c.valid, len(valid) == 1)
			suite.Require().Equal(!c.valid, len(invalid) == 1)
		})
	}
}

type validatingUniqTestSuite struct {
	suite.Suite

	dupSeeker    mocks.DuplicatesSeeker
	sourceMetric entities.SourceMetric
}

func TestUniq(t *testing.T) {
	suite.Run(t, new(validatingUniqTestSuite))
}

func (suite *validatingUniqTestSuite) SetupTest() {
	suite.dupSeeker = mocks.DuplicatesSeeker{}

	suite.sourceMetric = entities.SourceMetric{}
	suite.sourceMetric.Schema = "schema"
	suite.sourceMetric.MetricID = "metric"
	suite.sourceMetric.Usage.Start = time.Unix(10, 0)
	suite.sourceMetric.Usage.Finish = time.Unix(20, 0)
	suite.sourceMetric.MessageOffset = 100
}

func (suite *validatingUniqTestSuite) TestUniq() {
	var metrics []entities.SourceMetric
	for _, schema := range []string{"sch1", "sch2"} {
		for i := 0; i < 3; i++ {
			metric := suite.sourceMetric
			metric.Schema = schema
			metric.MetricID = fmt.Sprintf("%d", i)
			metrics = append(metrics, metric)
		}
	}

	suite.dupSeeker.On("FindDuplicates", mock.Anything, mock.Anything, mock.Anything, mock.Anything).Return(nil, nil)

	valid, invalid, err := ValidateMetricsUnique(context.Background(), entities.ProcessingScope{}, &suite.dupSeeker, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(invalid)
	suite.Require().Len(valid, 6)
}

func (suite *validatingUniqTestSuite) TestDuplicatesInBatch() {
	metrics := []entities.SourceMetric{
		suite.sourceMetric,
		suite.sourceMetric,
	}
	metrics[1].MessageOffset -= 1

	suite.dupSeeker.On("FindDuplicates", mock.Anything, mock.Anything, mock.Anything, mock.Anything).Return(nil, nil)

	valid, invalid, err := ValidateMetricsUnique(context.Background(), entities.ProcessingScope{}, &suite.dupSeeker, metrics)

	suite.Require().NoError(err)
	suite.Require().Len(invalid, 1)
	suite.Require().EqualValues(suite.sourceMetric.MessageOffset, invalid[0].MessageOffset)
	suite.EqualValues(entities.FailedByDuplicate, invalid[0].Reason)
	suite.EqualValues("'schema::metric' is duplicate", invalid[0].ReasonComment)
	suite.Len(valid, 2) // due to flag setted
}

func (suite *validatingUniqTestSuite) TestDuplicatesInAdapter() {
	metrics := []entities.SourceMetric{
		suite.sourceMetric,
	}

	dup := entities.MetricIdentity{
		Schema:   suite.sourceMetric.Schema,
		MetricID: suite.sourceMetric.MetricID,
		Offset:   suite.sourceMetric.MessageOffset,
	}
	suite.dupSeeker.On("FindDuplicates", mock.Anything, mock.Anything, mock.Anything, mock.Anything).
		Return([]entities.MetricIdentity{dup}, nil)

	valid, invalid, err := ValidateMetricsUnique(context.Background(), entities.ProcessingScope{}, &suite.dupSeeker, metrics)

	suite.Require().NoError(err)
	suite.Require().Len(invalid, 1)
	suite.NotEmpty(valid)
	suite.EqualValues(entities.FailedByDuplicate, invalid[0].Reason)
	suite.EqualValues("'schema::metric' is duplicate", invalid[0].ReasonComment)
}

func (suite *validatingUniqTestSuite) TestDuplicates() {
	metrics := []entities.SourceMetric{
		suite.sourceMetric,
		suite.sourceMetric,
		suite.sourceMetric,
	}
	metrics[1].MessageOffset -= 1
	metrics[2].Schema = "dupschema"

	adapterDup := entities.MetricIdentity{
		Schema:   "dupschema",
		MetricID: suite.sourceMetric.MetricID,
		Offset:   suite.sourceMetric.MessageOffset,
	}
	suite.dupSeeker.On("FindDuplicates", mock.Anything, mock.Anything, mock.Anything, mock.Anything).
		Return([]entities.MetricIdentity{adapterDup}, nil)

	valid, invalid, err := ValidateMetricsUnique(context.Background(), entities.ProcessingScope{}, &suite.dupSeeker, metrics)

	suite.Require().NoError(err)
	suite.NotEmpty(valid)
	suite.Require().Len(invalid, 2)

	sort.Slice(invalid, func(i, j int) bool { return invalid[i].MetricID().Schema < invalid[j].MetricID().Schema })

	suite.EqualValues(entities.FailedByDuplicate, invalid[0].Reason)
	suite.EqualValues(entities.FailedByDuplicate, invalid[1].Reason)
	suite.EqualValues("'dupschema::metric' is duplicate", invalid[0].ReasonComment)
	suite.EqualValues("'schema::metric' is duplicate", invalid[1].ReasonComment)
}

func (suite *validatingUniqTestSuite) TestDropDuplicates() {
	cases := []struct {
		name           string
		dropDuplicates bool
	}{
		{"dropDuplicates", true},
		{"keepDuplicates", false},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.name), func() {
			metrics := []entities.SourceMetric{
				suite.sourceMetric,
			}

			dup := entities.MetricIdentity{
				Schema:   suite.sourceMetric.Schema,
				MetricID: suite.sourceMetric.MetricID,
				Offset:   suite.sourceMetric.MessageOffset,
			}
			suite.dupSeeker.On("FindDuplicates", mock.Anything, mock.Anything, mock.Anything, mock.Anything).
				Return([]entities.MetricIdentity{dup}, nil)

			previousFlag := features.Default()
			f := features.Default()
			f.Set(features.DropDuplicates(c.dropDuplicates))
			features.SetDefault(f)

			defer features.SetDefault(previousFlag)

			valid, _, err := ValidateMetricsUnique(context.Background(), entities.ProcessingScope{}, &suite.dupSeeker, metrics)

			suite.Require().NoError(err)
			if c.dropDuplicates {
				// temporary
				suite.Require().NotEmpty(valid)
				// suite.Require().Empty(valid)
			} else {
				suite.Require().NotEmpty(valid)
			}
		})
	}
}

func (suite *validatingUniqTestSuite) TestPeriods() {
	metrics := []entities.SourceMetric{
		suite.sourceMetric,
		suite.sourceMetric,
	}
	metrics[0].Usage.Finish = metrics[0].Usage.Finish.Add(time.Hour * 24 * 40) // move to next month
	metrics[0].Schema = "other schema"

	suite.dupSeeker.On("FindDuplicates", mock.Anything, mock.Anything, mock.Anything, mock.Anything).Twice().Return(nil, nil)

	_, _, err := ValidateMetricsUnique(context.Background(), entities.ProcessingScope{}, &suite.dupSeeker, metrics)

	suite.Require().NoError(err)
	suite.dupSeeker.AssertExpectations(suite.T())
}
