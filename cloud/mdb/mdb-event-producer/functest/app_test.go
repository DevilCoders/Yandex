package functest

import (
	"path"
	"testing"

	"github.com/DATA-DOG/godog"

	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/library/go/test/yatest"
)

const featuresBaseDir = "cloud/mdb/mdb-event-producer/functest/features"

func FeatureContext(s *godog.Suite) {
	tc, err := initTestContext()
	if err != nil {
		panic(err.Error())
	}

	s.Step(`^cloud, folder, "(\d+)" clusters and tasks with events$`, tc.cloudFolderAndClustersAndTasks)
	s.Step(`^I create cloud, folder, "([^"]*)" clusters and tasks with events$`, tc.cloudFolderAndClustersAndTasks)
	s.Step(`^I run one mdb-event-producer$`, tc.iRunOneProducer)
	s.Step(`^I run "(\d+)" mdb-event-producers$`, tc.iRunProducers)
	s.Step(`^I finish my tasks$`, tc.iFinishMyTasks)
	s.Step(`^I fail my tasks$`, tc.iFallMyTasks)
	s.Step(`^my (start|done) events are sent in "([^"]*)"$`, tc.myEventsAreSent)
	s.Step(`^my done events are not sent in "([^"]*)"$`, tc.myDoneEventsAreNotSent)
	s.Step(`^I run mdb-event-queue-unsent with warn="(\w+)" crit="(\w+)"$`, tc.iRunMonitoring)
	s.Step(`^monitoring returns "(\w+)"$`, tc.monitoringReturns)
	s.Step(`^I sleep "(\w+)"$`, tc.iSleep)

	s.BeforeScenario(tc.BeforeScenario)
	s.AfterScenario(tc.AfterScenario)
}

func initializeSuite(s *godog.Suite) {
	FeatureContext(s)
}

func pathToFeature(featureFile string) string {
	rootPath := yatest.SourcePath(featuresBaseDir)
	return path.Join(rootPath, featureFile)
}

func TestConcurrent(t *testing.T) {
	godogutil.MakeSuiteFromFeatureMust(
		pathToFeature("concurrent.feature"),
		initializeSuite,
		t,
	)
}

func TestOneProducer(t *testing.T) {
	godogutil.MakeSuiteFromFeatureMust(
		pathToFeature("one.feature"),
		initializeSuite,
		t,
	)
}

func TestMonitoring(t *testing.T) {
	godogutil.MakeSuiteFromFeatureMust(
		pathToFeature("monitoring.feature"),
		initializeSuite,
		t,
	)
}
