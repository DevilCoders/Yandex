package zapjournald

import (
	"testing"

	"github.com/coreos/go-systemd/journal"
	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

type logSuite struct {
	suite.Suite
	logger log.Logger

	calls []sendCall
}

func TestLog(t *testing.T) {
	suite.Run(t, new(logSuite))
}

func (suite *logSuite) SetupTest() {
	core := NewCore(zapcore.DebugLevel)
	core.send = suite.send

	suite.logger = zap.NewWithCore(core)

	suite.calls = suite.calls[:0]
}

func (suite *logSuite) TestLog() {
	suite.logger.Info("test message", log.String("test", "value"))

	suite.Require().Len(suite.calls, 1)

	call := suite.calls[0]
	suite.Equal("test message", call.message)
	suite.Equal(journal.PriInfo, call.prio)
	suite.Len(call.vars, 1)
	suite.Equal("value", call.vars["TEST"])
}

func (suite *logSuite) TestPrio() {
	suite.logger.Debug("test")
	suite.logger.Info("test")
	suite.logger.Warn("test")
	suite.logger.Error("test")

	want := []sendCall{
		{message: "test", prio: journal.PriDebug},
		{message: "test", prio: journal.PriInfo},
		{message: "test", prio: journal.PriWarning},
		{message: "test", prio: journal.PriErr},
	}
	suite.ElementsMatch(want, suite.calls)
}

func (suite *logSuite) send(msg string, prio journal.Priority, vars map[string]string) error {
	suite.calls = append(suite.calls, sendCall{msg, prio, vars})
	return nil
}

type sendCall struct {
	message string
	prio    journal.Priority
	vars    map[string]string
}
