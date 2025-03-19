package updater

import (
	"context"
	"crypto/sha256"
	"errors"
	"fmt"
	"io"
	"net/http"
	"path/filepath"
	"runtime"
	"testing"

	"github.com/jarcoal/httpmock"
	"github.com/spf13/afero"
	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
	"go.uber.org/zap"
	"go.uber.org/zap/zaptest"

	"a.yandex-team.ru/cloud/marketplace/agent/pkg/logger"
	"a.yandex-team.ru/cloud/marketplace/agent/pkg/repo"
	"a.yandex-team.ru/cloud/marketplace/agent/windows/internal/guest"
	"a.yandex-team.ru/cloud/marketplace/agent/windows/internal/updater/mocks"
)

func TestUpdater(t *testing.T) {
	suite.Run(t, new(updaterTests))
	suite.Run(t, new(joinWithDotsTableTests))
}

type updaterTests struct {
	suite.Suite
	mocked  *mocked
	updater *GuestAgent
}

func (s *updaterTests) SetupTest() {
	s.mocked = initMocks(s.T())

	var err error
	s.updater, err = New(s.mocked.ctx)
	s.Require().NoError(err)
	s.Require().NotNil(s.updater)

	s.updater.fs = afero.NewMemMapFs()
}

type mocked struct {
	ctx                                                                                  context.Context
	ctxCanceled                                                                          context.Context
	initAgentExistFS                                                                     func(fs afero.Fs)
	initAgentNotExistFS                                                                  func(fs afero.Fs)
	initAgentDirExistFS                                                                  func(fs afero.Fs)
	initAgentFileInsteadOfDirExistFS                                                     func(fs afero.Fs)
	fileRepoOnInitNil                                                                    *mocks.Repository
	fileRepoOnInitErr                                                                    *mocks.Repository
	svcMgrOnInitNil                                                                      *mocks.ServiceManager
	svcMgrOnInitErr                                                                      *mocks.ServiceManager
	svcMgrNoop                                                                           *mocks.ServiceManager
	svcMgrOnExistErr                                                                     *mocks.ServiceManager
	svcMgrOnExistFalseOnExistErr                                                         *mocks.ServiceManager
	svcMgrOnExistFalseOnExistFalseOnCreateErr                                            *mocks.ServiceManager
	svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartErr                                  *mocks.ServiceManager
	svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartNil                                  *mocks.ServiceManager
	svcMgrOnExistTrueOnStoppedErr                                                        *mocks.ServiceManager
	svcMgrOnExistTrueOnStoppedTrueOnDeleteErr                                            *mocks.ServiceManager
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistErr                                  *mocks.ServiceManager
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateErr                     *mocks.ServiceManager
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartErr           *mocks.ServiceManager
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartNil           *mocks.ServiceManager
	svcMgrOnExistTrueOnStoppedFalseOnStopErr                                             *mocks.ServiceManager
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteErr                                  *mocks.ServiceManager
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistErr                        *mocks.ServiceManager
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateErr           *mocks.ServiceManager
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartErr *mocks.ServiceManager
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartNil *mocks.ServiceManager
	svcMgrOnIsExistNil                                                                   *mocks.ServiceManager
	svcMgrOnIsExistErr                                                                   *mocks.ServiceManager
	svcMgrOnIsExistFalseOnCreateNil                                                      *mocks.ServiceManager
	svcMgrOnIsExistFalseOnCreateErr                                                      *mocks.ServiceManager
	svcMgrOnIsExistFalse                                                                 *mocks.ServiceManager
	svcMgrOnIsExistTrueOnIsStoppedErr                                                    *mocks.ServiceManager
	svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteNil                                        *mocks.ServiceManager
	svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteErr                                        *mocks.ServiceManager
	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopErr                                         *mocks.ServiceManager
	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteErr                              *mocks.ServiceManager
	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteNil                              *mocks.ServiceManager
	svcMgrOnStartNil                                                                     *mocks.ServiceManager
	svcMgrOnStartErr                                                                     *mocks.ServiceManager
	svcMgrOnStopNil                                                                      *mocks.ServiceManager
	svcMgrOnStopErr                                                                      *mocks.ServiceManager
	svcMgrOnCloseNil                                                                     *mocks.ServiceManager
	svcMgrOnCloseErr                                                                     *mocks.ServiceManager
}

func initMocks(t *testing.T) *mocked {
	ctx := initTestingLogger(t)
	ctxCanceled, cancel := context.WithCancel(initTestingLogger(t))
	cancel()

	agentFilepath := filepath.Join(guest.AgentDir, guest.AgentExecutable)
	initAgentExistFS := func(fs afero.Fs) {
		require.NoError(t, fs.Mkdir(guest.AgentDir, defaultPerms))
		_, err := fs.Create(agentFilepath)
		require.NoError(t, err)
	}
	initAgentDirExistFS := func(fs afero.Fs) {
		require.NoError(t, fs.Mkdir(guest.AgentDir, defaultPerms))
	}
	initAgentFileInsteadOfDirExistFS := func(fs afero.Fs) {
		_, err := fs.Create(guest.AgentDir)
		require.NoError(t, err)
	}

	fileRepoOnInitNil := mocks.Repository{}
	fileRepoOnInitNil.On("Init").Return(nil)

	fileRepoOnInitErr := mocks.Repository{}
	fileRepoOnInitErr.On("Init").Return(errors.New("any"))

	svcMgrNoop := mocks.ServiceManager{}

	svcMgrOnInitNil := mocks.ServiceManager{}
	svcMgrOnInitNil.On("Init").Return(nil)

	svcMgrOnInitErr := mocks.ServiceManager{}
	svcMgrOnInitErr.On("Init").Return(errors.New("any"))

	svcMgrOnExistErr := mocks.ServiceManager{}
	svcMgrOnExistErr.On("IsExist", guest.ServiceName).Return(false, errors.New("any"))

	svcMgrOnExistFalseOnExistErr := mocks.ServiceManager{}
	svcMgrOnExistFalseOnExistErr.On("IsExist", guest.ServiceName).Return(false, nil).Once()
	svcMgrOnExistFalseOnExistErr.On("IsExist", guest.ServiceName).Return(false, errors.New("any"))

	svcMgrOnExistFalseOnExistFalseOnCreateErr := mocks.ServiceManager{}
	svcMgrOnExistFalseOnExistFalseOnCreateErr.On("IsExist", guest.ServiceName).Return(false, nil).Times(2)
	svcMgrOnExistFalseOnExistFalseOnCreateErr.On("Create", agentFilepath, guest.ServiceName, guest.ServiceName, guest.ServiceDescription, guest.ServiceArgs).Return(errors.New("any"))

	svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartErr := mocks.ServiceManager{}
	svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartErr.On("IsExist", guest.ServiceName).Return(false, nil).Times(2)
	svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartErr.On("Create", agentFilepath, guest.ServiceName, guest.ServiceName, guest.ServiceDescription, guest.ServiceArgs).Return(nil)
	svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartErr.On("Start", guest.ServiceName).Return(errors.New("any"))

	svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartNil := mocks.ServiceManager{}
	svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartNil.On("IsExist", guest.ServiceName).Return(false, nil).Times(2)
	svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartNil.On("Create", agentFilepath, guest.ServiceName, guest.ServiceName, guest.ServiceDescription, guest.ServiceArgs).Return(nil)
	svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartNil.On("Start", guest.ServiceName).Return(nil)

	svcMgrOnExistTrueOnStoppedErr := mocks.ServiceManager{}
	svcMgrOnExistTrueOnStoppedErr.On("IsExist", guest.ServiceName).Return(true, nil)
	svcMgrOnExistTrueOnStoppedErr.On("IsStopped", guest.ServiceName).Return(false, errors.New("any"))

	svcMgrOnExistTrueOnStoppedTrueOnDeleteErr := mocks.ServiceManager{}
	svcMgrOnExistTrueOnStoppedTrueOnDeleteErr.On("IsExist", guest.ServiceName).Return(true, nil)
	svcMgrOnExistTrueOnStoppedTrueOnDeleteErr.On("IsStopped", guest.ServiceName).Return(true, nil)
	svcMgrOnExistTrueOnStoppedTrueOnDeleteErr.On("Delete", guest.ServiceName).Return(errors.New("any"))

	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistErr := mocks.ServiceManager{}
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistErr.On("IsExist", guest.ServiceName).Return(true, nil).Once()
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistErr.On("IsStopped", guest.ServiceName).Return(true, nil)
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistErr.On("Delete", guest.ServiceName).Return(nil)
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistErr.On("IsExist", guest.ServiceName).Return(false, errors.New("any"))

	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateErr := mocks.ServiceManager{}
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateErr.On("IsExist", guest.ServiceName).Return(true, nil).Once()
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateErr.On("IsStopped", guest.ServiceName).Return(true, nil)
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateErr.On("Delete", guest.ServiceName).Return(nil)
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateErr.On("IsExist", guest.ServiceName).Return(false, nil)
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateErr.On("Create", agentFilepath, guest.ServiceName, guest.ServiceName, guest.ServiceDescription, guest.ServiceArgs).Return(errors.New("any"))

	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartErr := mocks.ServiceManager{}
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartErr.On("IsExist", guest.ServiceName).Return(true, nil).Once()
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartErr.On("IsStopped", guest.ServiceName).Return(true, nil)
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartErr.On("Delete", guest.ServiceName).Return(nil)
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartErr.On("IsExist", guest.ServiceName).Return(false, nil)
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartErr.On("Create", agentFilepath, guest.ServiceName, guest.ServiceName, guest.ServiceDescription, guest.ServiceArgs).Return(nil)
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartErr.On("Start", guest.ServiceName).Return(errors.New("any"))

	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartNil := mocks.ServiceManager{}
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartNil.On("IsExist", guest.ServiceName).Return(true, nil).Once()
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartNil.On("IsStopped", guest.ServiceName).Return(true, nil)
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartNil.On("Delete", guest.ServiceName).Return(nil)
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartNil.On("IsExist", guest.ServiceName).Return(false, nil)
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartNil.On("Create", agentFilepath, guest.ServiceName, guest.ServiceName, guest.ServiceDescription, guest.ServiceArgs).Return(nil)
	svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartNil.On("Start", guest.ServiceName).Return(nil)

	svcMgrOnExistTrueOnStoppedFalseOnStopErr := mocks.ServiceManager{}
	svcMgrOnExistTrueOnStoppedFalseOnStopErr.On("IsExist", guest.ServiceName).Return(true, nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopErr.On("IsStopped", guest.ServiceName).Return(false, nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopErr.On("Stop", guest.ServiceName).Return(errors.New("any"))

	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteErr := mocks.ServiceManager{}
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteErr.On("IsExist", guest.ServiceName).Return(true, nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteErr.On("IsStopped", guest.ServiceName).Return(false, nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteErr.On("Stop", guest.ServiceName).Return(nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteErr.On("Delete", guest.ServiceName).Return(errors.New("any"))

	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistErr := mocks.ServiceManager{}
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistErr.On("IsExist", guest.ServiceName).Return(true, nil).Once()
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistErr.On("IsStopped", guest.ServiceName).Return(false, nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistErr.On("Stop", guest.ServiceName).Return(nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistErr.On("Delete", guest.ServiceName).Return(nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistErr.On("IsExist", guest.ServiceName).Return(false, errors.New("any"))

	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateErr := mocks.ServiceManager{}
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateErr.On("IsExist", guest.ServiceName).Return(true, nil).Once()
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateErr.On("IsStopped", guest.ServiceName).Return(false, nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateErr.On("Stop", guest.ServiceName).Return(nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateErr.On("Delete", guest.ServiceName).Return(nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateErr.On("IsExist", guest.ServiceName).Return(false, nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateErr.On("Create", agentFilepath, guest.ServiceName, guest.ServiceName, guest.ServiceDescription, guest.ServiceArgs).Return(errors.New("any"))

	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartErr := mocks.ServiceManager{}
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartErr.On("IsExist", guest.ServiceName).Return(true, nil).Once()
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartErr.On("IsStopped", guest.ServiceName).Return(false, nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartErr.On("Stop", guest.ServiceName).Return(nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartErr.On("Delete", guest.ServiceName).Return(nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartErr.On("IsExist", guest.ServiceName).Return(false, nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartErr.On("Create", agentFilepath, guest.ServiceName, guest.ServiceName, guest.ServiceDescription, guest.ServiceArgs).Return(nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartErr.On("Start", guest.ServiceName).Return(errors.New("any"))

	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartNil := mocks.ServiceManager{}
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartNil.On("IsExist", guest.ServiceName).Return(true, nil).Once()
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartNil.On("IsStopped", guest.ServiceName).Return(false, nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartNil.On("Stop", guest.ServiceName).Return(nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartNil.On("Delete", guest.ServiceName).Return(nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartNil.On("IsExist", guest.ServiceName).Return(false, nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartNil.On("Create", agentFilepath, guest.ServiceName, guest.ServiceName, guest.ServiceDescription, guest.ServiceArgs).Return(nil)
	svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartNil.On("Start", guest.ServiceName).Return(nil)

	svcMgrOnIsExistErr := mocks.ServiceManager{}
	svcMgrOnIsExistErr.On("IsExist", guest.ServiceName).Return(false, errors.New("any"))

	svcMgrOnIsExistNil := mocks.ServiceManager{}
	svcMgrOnIsExistNil.On("IsExist", guest.ServiceName).Return(true, nil)

	svcMgrOnIsExistFalseOnCreateNil := mocks.ServiceManager{}
	svcMgrOnIsExistFalseOnCreateNil.On("IsExist", guest.ServiceName).Return(false, nil)
	svcMgrOnIsExistFalseOnCreateNil.On("Create", agentFilepath, guest.ServiceName, guest.ServiceName, guest.ServiceDescription, guest.ServiceArgs).Return(nil)

	svcMgrOnIsExistFalseOnCreateErr := mocks.ServiceManager{}
	svcMgrOnIsExistFalseOnCreateErr.On("IsExist", guest.ServiceName).Return(false, nil)
	svcMgrOnIsExistFalseOnCreateErr.On("Create", agentFilepath, guest.ServiceName, guest.ServiceName, guest.ServiceDescription, guest.ServiceArgs).Return(errors.New("any"))

	svcMgrOnIsExistFalse := mocks.ServiceManager{}
	svcMgrOnIsExistFalse.On("IsExist", guest.ServiceName).Return(false, nil)

	svcMgrOnIsExistTrueOnIsStoppedErr := mocks.ServiceManager{}
	svcMgrOnIsExistTrueOnIsStoppedErr.On("IsExist", guest.ServiceName).Return(true, nil)
	svcMgrOnIsExistTrueOnIsStoppedErr.On("IsStopped", guest.ServiceName).Return(false, errors.New("any"))

	svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteNil := mocks.ServiceManager{}
	svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteNil.On("IsExist", guest.ServiceName).Return(true, nil)
	svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteNil.On("IsStopped", guest.ServiceName).Return(true, nil)
	svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteNil.On("Delete", guest.ServiceName).Return(nil)

	svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteErr := mocks.ServiceManager{}
	svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteErr.On("IsExist", guest.ServiceName).Return(true, nil)
	svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteErr.On("IsStopped", guest.ServiceName).Return(true, nil)
	svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteErr.On("Delete", guest.ServiceName).Return(errors.New("any"))

	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopErr := mocks.ServiceManager{}
	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopErr.On("IsExist", guest.ServiceName).Return(true, nil)
	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopErr.On("IsStopped", guest.ServiceName).Return(false, nil)
	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopErr.On("Stop", guest.ServiceName).Return(errors.New("any"))

	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteErr := mocks.ServiceManager{}
	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteErr.On("IsExist", guest.ServiceName).Return(true, nil)
	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteErr.On("IsStopped", guest.ServiceName).Return(false, nil)
	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteErr.On("Stop", guest.ServiceName).Return(nil)
	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteErr.On("Delete", guest.ServiceName).Return(errors.New("any"))

	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteNil := mocks.ServiceManager{}
	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteNil.On("IsExist", guest.ServiceName).Return(true, nil)
	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteNil.On("IsStopped", guest.ServiceName).Return(false, nil)
	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteNil.On("Stop", guest.ServiceName).Return(nil)
	svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteNil.On("Delete", guest.ServiceName).Return(nil)

	svcMgrOnStartNil := mocks.ServiceManager{}
	svcMgrOnStartNil.On("Start", guest.ServiceName).Return(nil)

	svcMgrOnStartErr := mocks.ServiceManager{}
	svcMgrOnStartErr.On("Start", guest.ServiceName).Return(errors.New("any"))
	svcMgrOnStopNil := mocks.ServiceManager{}
	svcMgrOnStopNil.On("Stop", guest.ServiceName).Return(nil)

	svcMgrOnStopErr := mocks.ServiceManager{}
	svcMgrOnStopErr.On("Stop", guest.ServiceName).Return(errors.New("any"))

	svcMgrOnCloseNil := mocks.ServiceManager{}
	svcMgrOnCloseNil.On("Close").Return(nil)

	svcMgrOnCloseErr := mocks.ServiceManager{}
	svcMgrOnCloseErr.On("Close").Return(errors.New("any"))

	return &mocked{
		ctx:                              ctx,
		ctxCanceled:                      ctxCanceled,
		initAgentExistFS:                 initAgentExistFS,
		initAgentNotExistFS:              func(_ afero.Fs) {},
		initAgentDirExistFS:              initAgentDirExistFS,
		initAgentFileInsteadOfDirExistFS: initAgentFileInsteadOfDirExistFS,
		fileRepoOnInitNil:                &fileRepoOnInitNil,
		fileRepoOnInitErr:                &fileRepoOnInitErr,
		svcMgrOnInitNil:                  &svcMgrOnInitNil,
		svcMgrOnInitErr:                  &svcMgrOnInitErr,
		svcMgrNoop:                       &svcMgrNoop,
		svcMgrOnExistErr:                 &svcMgrOnExistErr,
		svcMgrOnExistFalseOnExistErr:     &svcMgrOnExistFalseOnExistErr,
		svcMgrOnExistFalseOnExistFalseOnCreateErr:                                            &svcMgrOnExistFalseOnExistFalseOnCreateErr,
		svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartErr:                                  &svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartErr,
		svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartNil:                                  &svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartNil,
		svcMgrOnExistTrueOnStoppedErr:                                                        &svcMgrOnExistTrueOnStoppedErr,
		svcMgrOnExistTrueOnStoppedTrueOnDeleteErr:                                            &svcMgrOnExistTrueOnStoppedTrueOnDeleteErr,
		svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistErr:                                  &svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistErr,
		svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateErr:                     &svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateErr,
		svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartErr:           &svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartErr,
		svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartNil:           &svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartNil,
		svcMgrOnExistTrueOnStoppedFalseOnStopErr:                                             &svcMgrOnExistTrueOnStoppedFalseOnStopErr,
		svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteErr:                                  &svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteErr,
		svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistErr:                        &svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistErr,
		svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateErr:           &svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateErr,
		svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartErr: &svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartErr,
		svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartNil: &svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartNil,
		svcMgrOnIsExistNil:                                      &svcMgrOnIsExistNil,
		svcMgrOnIsExistErr:                                      &svcMgrOnIsExistErr,
		svcMgrOnIsExistFalseOnCreateNil:                         &svcMgrOnIsExistFalseOnCreateNil,
		svcMgrOnIsExistFalseOnCreateErr:                         &svcMgrOnIsExistFalseOnCreateErr,
		svcMgrOnIsExistFalse:                                    &svcMgrOnIsExistFalse,
		svcMgrOnIsExistTrueOnIsStoppedErr:                       &svcMgrOnIsExistTrueOnIsStoppedErr,
		svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteNil:           &svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteNil,
		svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteErr:           &svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteErr,
		svcMgrOnIsExistTrueOnIsStoppedFalseOnStopErr:            &svcMgrOnIsExistTrueOnIsStoppedFalseOnStopErr,
		svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteErr: &svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteErr,
		svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteNil: &svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteNil,
		svcMgrOnStartNil:                                        &svcMgrOnStartNil,
		svcMgrOnStartErr:                                        &svcMgrOnStartErr,
		svcMgrOnStopNil:                                         &svcMgrOnStopNil,
		svcMgrOnStopErr:                                         &svcMgrOnStopErr,
		svcMgrOnCloseNil:                                        &svcMgrOnCloseNil,
		svcMgrOnCloseErr:                                        &svcMgrOnCloseErr,
	}
}

func initTestingLogger(t *testing.T) context.Context {
	l := zaptest.NewLogger(t)
	ctx := context.Background()

	return logger.NewContext(ctx, l)
}

func (s *updaterTests) TestNew() {
	for n, t := range []struct {
		ctx     context.Context
		wantErr error
	}{
		{nil, errors.New("provided nil context")},
		{s.mocked.ctx, nil},
		{s.mocked.ctxCanceled, context.Canceled},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		u, err := New(t.ctx)

		if t.wantErr != nil {
			s.Nil(u)
			s.Error(err)
		} else {
			s.NotNil(u)
			s.NoError(err)
		}
	}
}

func (s *updaterTests) TestInit() {
	for n, t := range []struct {
		fileRepo *mocks.Repository
		svcMgr   *mocks.ServiceManager
		ctx      context.Context
		wantErr  error
	}{
		{s.mocked.fileRepoOnInitNil, s.mocked.svcMgrOnInitNil, s.mocked.ctx, nil},
		{s.mocked.fileRepoOnInitNil, s.mocked.svcMgrOnInitNil, s.mocked.ctxCanceled, errors.New("any")},
		{s.mocked.fileRepoOnInitNil, s.mocked.svcMgrOnInitErr, nil, errors.New("any")},
		{s.mocked.fileRepoOnInitErr, s.mocked.svcMgrNoop, nil, errors.New("any")},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		s.updater.ctx = t.ctx
		s.updater.fileRepo = t.fileRepo
		s.updater.svcMgr = t.svcMgr

		err := s.updater.Init()

		if t.wantErr != nil {
			s.Error(err, n)
		} else {
			s.NoError(err, n)
		}

		t.svcMgr.AssertExpectations(s.T())
		t.fileRepo.AssertExpectations(s.T())
		s.updater.fs = afero.NewMemMapFs()
	}
}

func (s *updaterTests) createDummyVersionInRepository(fs afero.Fs, version string) {
	if version == "" {
		return
	}

	dummyVersion := filepath.Join(versionLocalRepository, version, guest.AgentExecutable)
	err := afero.WriteFile(fs, dummyVersion, []byte("I R AGENT"), defaultPerms)
	logger.DebugCtx(s.mocked.ctx, err, "write file",
		zap.String("path", dummyVersion),
		zap.String("versionLocalRepository", versionLocalRepository),
		zap.String("guest agent exe", guest.AgentExecutable),
		zap.String("version", version))
	s.Require().NoError(err)

	f, err := fs.Open(dummyVersion)
	logger.DebugCtx(s.mocked.ctx, err, "open file",
		zap.String("path", dummyVersion))
	s.Require().NoError(err)
	defer func() { _ = f.Close() }()

	h := sha256.New()
	_, err = io.Copy(h, f)
	s.Require().NoError(err)
	hash := fmt.Sprintf("%x", h.Sum(nil))

	dummyChecksum := joinWithDots(dummyVersion, checksumSuffix)
	err = afero.WriteFile(fs, dummyChecksum, []byte(hash), defaultPerms)
	logger.DebugCtx(s.mocked.ctx, err, "write file",
		zap.String("path", dummyChecksum),
		zap.String("hash", hash))
	s.Require().NoError(err)
}

func (s *updaterTests) TestCheck() {
	for n, t := range []struct {
		ctx       context.Context
		initFs    func(fs afero.Fs)
		installed string
		repo      string
		online    string
		wantState State
		wantErr   error
	}{
		{s.mocked.ctxCanceled, s.mocked.initAgentNotExistFS, "", "", "", Unknown, errors.New("any")},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "0.0.1", "0.0.1", "0.0.1", Noop, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "0.0.1", "", "", Noop, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "0.0.1", "0.0.1", "", Noop, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "0.0.1", "0.0.2", "0.0.2", Update, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "0.0.1", "0.0.2", "0.0.1", DownloadAndUpdate, nil}, // download to repository, coz only 0.0.2 in repo
		{s.mocked.ctx, s.mocked.initAgentExistFS, "0.0.1", "0.0.2", "", Update, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "0.0.2", "0.0.1", "0.0.2", Download, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "0.0.1", "0.0.1", "0.0.2", DownloadAndUpdate, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "", "0.0.2", "0.0.2", Install, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "", "0.0.2", "0.0.1", DownloadAndInstall, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "", "0.0.2", "", Install, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "", "", "0.0.2", DownloadAndInstall, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "", "0.0.1", "0.0.2", DownloadAndInstall, nil},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		s.updater.ctx = t.ctx

		// mock repo
		s.updater.fs = afero.NewMemMapFs()
		s.createDummyVersionInRepository(s.updater.fs, t.repo)
		r, err := repo.NewFiler(s.mocked.ctx, versionLocalRepository, guest.AgentExecutable, s.updater.fs)
		s.Require().NoError(err)
		s.updater.fileRepo = r
		s.Require().NoError(s.updater.fileRepo.Init())

		// mock installed agent
		s.updater.getAgentVersion = func(_ string) (string, error) { return t.installed, nil }
		t.initFs(s.updater.fs)

		// mock online
		resp := httpmock.NewStringResponder(http.StatusOK, t.online)
		httpmock.RegisterResponder(http.MethodGet, guestAgentLatest, resp)
		httpmock.ActivateNonDefault(s.updater.hclient.GetClient())

		state, err := s.updater.Check()

		if t.wantErr != nil {
			s.Error(err)
			s.Equal(t.wantState, state)
		} else {
			s.NoError(err)
			s.Equal(t.wantState, state)
		}

		httpmock.DeactivateAndReset()
	}
}

// +failed ctx
// +noop
// +update that actually install
// +update only version
// +update only version failed
// update with rollback
// update with rollback failed
// Update()
//		err := u.ensureLatestAdded() // http.StatusOK + repoVersion
//		repoVersion := u.getRepoLatest()
//		instVersion, err := u.getInstalledVersion()
//		instVersion != ""
//			installed, err := semver.New(instVersion)
//			maybeLatest, err := semver.New(repoVersion)
//			alreadyLatest := installed.GE(*maybeLatest)
//		err = u.ctx.Err()
//		err = u.install(repoVersion)
//		if err != nil
//			prevRepoVersion := u.getRepoPrevious(repoVersion)
//			prevRepoVersion != ""
//			err = u.install(prevRepoVersion)

func (s *updaterTests) TestUpdate() {
	agentFilepath := filepath.Join(guest.AgentDir, guest.AgentExecutable)

	svcMgrUpdateWithRollbackOk := mocks.ServiceManager{}
	svcMgrUpdateWithRollbackOk.On("IsExist", guest.ServiceName).Return(true, nil).Once()
	svcMgrUpdateWithRollbackOk.On("IsStopped", guest.ServiceName).Return(false, nil)
	svcMgrUpdateWithRollbackOk.On("Stop", guest.ServiceName).Return(nil)
	svcMgrUpdateWithRollbackOk.On("Delete", guest.ServiceName).Return(nil)
	svcMgrUpdateWithRollbackOk.On("IsExist", guest.ServiceName).Return(false, nil)
	svcMgrUpdateWithRollbackOk.On("Create", agentFilepath, guest.ServiceName, guest.ServiceName, guest.ServiceDescription, guest.ServiceArgs).Return(nil)
	svcMgrUpdateWithRollbackOk.On("Start", guest.ServiceName).Return(errors.New("any")).Once()
	svcMgrUpdateWithRollbackOk.On("Start", guest.ServiceName).Return(nil)

	svcMgrUpdateWithRollbackFailed := mocks.ServiceManager{}
	svcMgrUpdateWithRollbackFailed.On("IsExist", guest.ServiceName).Return(true, nil).Once()
	svcMgrUpdateWithRollbackFailed.On("IsStopped", guest.ServiceName).Return(false, nil)
	svcMgrUpdateWithRollbackFailed.On("Stop", guest.ServiceName).Return(nil)
	svcMgrUpdateWithRollbackFailed.On("Delete", guest.ServiceName).Return(nil)
	svcMgrUpdateWithRollbackFailed.On("IsExist", guest.ServiceName).Return(false, nil)
	svcMgrUpdateWithRollbackFailed.On("Create", agentFilepath, guest.ServiceName, guest.ServiceName, guest.ServiceDescription, guest.ServiceArgs).Return(nil)
	svcMgrUpdateWithRollbackFailed.On("Start", guest.ServiceName).Return(errors.New("any"))

	for n, t := range []struct {
		ctx       context.Context
		initFs    func(fs afero.Fs)
		installed string
		repo      []string
		svcMgr    *mocks.ServiceManager
		wantErr   error
	}{
		{s.mocked.ctxCanceled, s.mocked.initAgentExistFS, "0.0.1", []string{"0.0.1"}, s.mocked.svcMgrNoop, errors.New("any")},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "0.0.1", []string{"0.0.1"}, s.mocked.svcMgrNoop, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "", []string{"0.0.1"}, s.mocked.svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartNil, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "", []string{"0.0.1"}, s.mocked.svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartErr, errors.New("any")},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "0.0.1", []string{"0.0.1", "0.0.2"}, s.mocked.svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartNil, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "0.0.1", []string{"0.0.1", "0.0.2"}, &svcMgrUpdateWithRollbackOk, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "0.0.1", []string{"0.0.1", "0.0.2"}, &svcMgrUpdateWithRollbackFailed, errors.New("any")},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		s.updater.ctx = t.ctx
		s.updater.svcMgr = t.svcMgr

		// mock http
		var resp httpmock.Responder
		if len(t.repo) > 0 {
			resp = httpmock.NewStringResponder(http.StatusOK, t.repo[0])
		} else {
			resp = httpmock.NewStringResponder(http.StatusOK, "")
		}
		httpmock.RegisterResponder(http.MethodGet, guestAgentLatest, resp)
		httpmock.ActivateNonDefault(s.updater.hclient.GetClient())

		// mock repo
		s.updater.fs = afero.NewMemMapFs()
		for _, v := range t.repo {
			s.createDummyVersionInRepository(s.updater.fs, v)
		}
		r, err := repo.NewFiler(s.mocked.ctx, versionLocalRepository, guest.AgentExecutable, s.updater.fs)
		s.Require().NoError(err)
		s.updater.fileRepo = r
		s.Require().NoError(s.updater.fileRepo.Init())

		// mock installed agent
		s.updater.getAgentVersion = func(_ string) (string, error) { return t.installed, nil }
		t.initFs(s.updater.fs)

		err = s.updater.Update()

		if t.wantErr != nil {
			s.Error(err)
		} else {
			s.NoError(err)
		}

		t.svcMgr.AssertExpectations(s.T())
		httpmock.DeactivateAndReset()
	}
}

func (s *updaterTests) TestInstall() {
	for n, t := range []struct {
		ctx     context.Context
		ver     string
		srvMgr  *mocks.ServiceManager
		wantErr error
	}{
		{s.mocked.ctxCanceled, "0.0.1", s.mocked.svcMgrNoop, errors.New("any")},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistErr, errors.New("any")},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistFalseOnExistErr, errors.New("any")},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistFalseOnExistFalseOnCreateErr, errors.New("any")},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartErr, errors.New("any")},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistFalseOnExistFalseOnCreateNilOnStartNil, nil},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistTrueOnStoppedErr, errors.New("any")},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistTrueOnStoppedTrueOnDeleteErr, errors.New("any")},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistErr, errors.New("any")},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateErr, errors.New("any")},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartErr, errors.New("any")},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistTrueOnStoppedTrueOnDeleteNilOnExistFalseOnCreateNilOnStartNil, nil},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistTrueOnStoppedFalseOnStopErr, errors.New("any")},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteErr, errors.New("any")},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistErr, errors.New("any")},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateErr, errors.New("any")},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartErr, errors.New("any")},
		{s.mocked.ctx, "0.0.1", s.mocked.svcMgrOnExistTrueOnStoppedFalseOnStopNilOnDeleteNilOnExistFalseOnCreateNilOnStartNil, nil},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		// mock repo
		s.updater.fs = afero.NewMemMapFs()
		s.createDummyVersionInRepository(s.updater.fs, "0.0.1")
		r, err := repo.NewFiler(s.mocked.ctx, versionLocalRepository, guest.AgentExecutable, s.updater.fs)
		s.Require().NoError(err)
		s.updater.fileRepo = r
		s.Require().NoError(s.updater.fileRepo.Init())

		// inject
		s.updater.ctx = t.ctx
		s.updater.svcMgr = t.srvMgr

		err = s.updater.install(t.ver)

		if t.wantErr != nil {
			s.Error(err)
		} else {
			s.NoError(err)
		}

		t.srvMgr.AssertExpectations(s.T())
	}
}

func (s *updaterTests) TestGetInstalledVersion() {
	for n, t := range []struct {
		ctx     context.Context
		initFS  func(fs afero.Fs)
		retVer  string
		retErr  error
		wantErr error
	}{
		{s.mocked.ctx, s.mocked.initAgentExistFS, "0.0.1", nil, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "", errors.New("any"), errors.New("any")},
		{s.mocked.ctx, s.mocked.initAgentNotExistFS, "", nil, nil},
		{s.mocked.ctx, s.mocked.initAgentNotExistFS, "", nil, nil},
		{s.mocked.ctxCanceled, s.mocked.initAgentExistFS, "0.0.1", nil, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.initAgentExistFS, "", errors.New("any"), errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.initAgentNotExistFS, "0.0.1", nil, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.initAgentNotExistFS, "", nil, errors.New("any")},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		s.updater.ctx = t.ctx
		s.updater.getAgentVersion = func(_ string) (string, error) {
			return t.retVer, t.retErr
		}
		t.initFS(s.updater.fs)

		gotVer, gotErr := s.updater.getInstalledVersion()
		if t.wantErr != nil {
			s.Error(gotErr)
			s.Empty(gotVer)
		} else {
			s.NoError(gotErr)
			s.Equal(t.retVer, gotVer)
		}
		s.updater.fs = afero.NewMemMapFs()
	}
}

func (s *updaterTests) TestGetRepoPreviosTo() {
	for n, t := range []struct {
		wantVer string
		passVer string
		list    []string
	}{
		{"", "0.0.5", []string{""}},
		{"", "0.0.5", []string{"0.0.5"}},
		{"", "0.0.5", []string{"0.0.5", "0.0.6"}},
		{"", "0.0.5", []string{"0.0.4", "0.0.6"}},
		{"0.0.4", "0.0.5", []string{"0.0.4", "0.0.5"}},
		{"0.0.4", "0.0.5", []string{"0.0.5", "0.0.4"}},
		{"0.0.4", "0.0.5", []string{"0.0.4", "0.0.5", "0.0.6"}},
		{"0.0.4", "0.0.5", []string{"0.0.6", "0.0.5", "0.0.4"}},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		f := mocks.Repository{}
		f.On("List").Return(t.list)
		s.updater.fileRepo = &f

		s.Equal(t.wantVer, s.updater.getRepoPrevious(t.passVer))
		f.AssertExpectations(s.T())
	}
}

func (s *updaterTests) TestGetRepoLatest() {
	for n, t := range []struct {
		list []string
		want string
	}{
		{[]string{""}, ""},
		{[]string{"0.0.1"}, "0.0.1"},
		{[]string{"0.0.1", "0.0.2"}, "0.0.2"},
		{[]string{"0.0.1", "0.0.2", "0.0.3"}, "0.0.3"},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		f := mocks.Repository{}
		f.On("List").Return(t.list)
		s.updater.fileRepo = &f

		s.Equal(t.want, s.updater.getRepoLatest())
		f.AssertExpectations(s.T())
	}
}

func (s *updaterTests) TestGetLatest() {
	for n, t := range []struct {
		ctx       context.Context
		retStatus int
		retVer    string
		wantVer   string
		wantErr   error
	}{
		{s.mocked.ctx, http.StatusOK, "0.0.1", "0.0.1", nil},
		{s.mocked.ctx, http.StatusOK, "", "", nil},
		{s.mocked.ctx, http.StatusOK, "terriblyWrongSemver", "", nil},
		{s.mocked.ctx, http.StatusInternalServerError, "", "", nil},
		{s.mocked.ctxCanceled, http.StatusOK, "0.0.1", "", errors.New("any")},
		{s.mocked.ctxCanceled, http.StatusOK, "", "", errors.New("any")},
		{s.mocked.ctxCanceled, http.StatusOK, "terriblyWrongSemver", "", errors.New("any")},
		{s.mocked.ctxCanceled, http.StatusInternalServerError, "", "", errors.New("any")},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		s.updater.ctx = t.ctx

		v := httpmock.NewStringResponder(t.retStatus, t.retVer)
		httpmock.RegisterResponder(http.MethodGet, guestAgentLatest, v)
		httpmock.ActivateNonDefault(s.updater.hclient.GetClient())

		gotVer, err := s.updater.getLatest()

		if t.wantErr != nil {
			s.Error(err)
			s.Equal(t.wantVer, gotVer)
		} else {
			s.NoError(err)
			s.Equal(t.wantVer, gotVer)
		}

		httpmock.DeactivateAndReset()
	}
}

func (s *updaterTests) TestDownloadVersion() {
	for n, t := range []struct {
		ctx     context.Context
		ver     string
		status  int
		wantErr error
	}{
		{s.mocked.ctx, "0.0.1", http.StatusOK, nil},
		{s.mocked.ctx, "0.0.1", http.StatusInternalServerError, errors.New("any")},
		{s.mocked.ctxCanceled, "0.0.1", http.StatusOK, errors.New("any")},
		{s.mocked.ctxCanceled, "0.0.1", http.StatusInternalServerError, errors.New("any")},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		// http mock
		urlAgent := fmt.Sprintf(`%v/yandexcloud-guestagent/release/%v/%v/%v/%v`,
			versionRemoteEndpoint, t.ver, runtime.GOOS, runtime.GOARCH, guest.AgentExecutable)
		respAgent := httpmock.NewStringResponder(t.status, "I R AGENT CONTENT")

		urlChecksum := joinWithDots(urlAgent, checksumSuffix)
		respChecksum := httpmock.NewStringResponder(t.status, "I R CHECKSUM")

		httpmock.RegisterResponder(http.MethodGet, urlAgent, respAgent)
		httpmock.RegisterResponder(http.MethodGet, urlChecksum, respChecksum)
		httpmock.ActivateNonDefault(s.updater.hclient.GetClient())

		// test
		s.updater.ctx = t.ctx
		path, err := s.updater.downloadVersion(t.ver)

		if t.wantErr != nil {
			s.Error(err)
			s.Empty(path)
		} else {
			s.NoError(err)

			exist, exErr := afero.Exists(s.updater.fs, path)
			s.NoError(exErr)
			s.True(exist)

			exist, exErr = afero.Exists(s.updater.fs, joinWithDots(path, checksumSuffix))
			s.NoError(exErr)
			s.True(exist)
		}

		httpmock.DeactivateAndReset()
		s.updater.fs = afero.NewMemMapFs()
	}
}

func (s *updaterTests) TestEnsureLatestAdded() {
	for n, t := range []struct {
		ctx        context.Context
		repoGetRet string
		repoAddRet error
		httpVer    string
		httpStatus int
		wantErr    error
	}{
		{s.mocked.ctx, "", nil, "0.0.1", http.StatusOK, nil},
		{s.mocked.ctx, "", nil, "", http.StatusInternalServerError, errors.New("any")},
		{s.mocked.ctx, "", errors.New("any"), "0.0.1", http.StatusOK, errors.New("any")},
		{s.mocked.ctx, "0.0.1", nil, "0.0.1", http.StatusOK, nil},
		{s.mocked.ctxCanceled, "", nil, "0.0.1", http.StatusOK, errors.New("any")},
		{s.mocked.ctxCanceled, "", nil, "", http.StatusInternalServerError, errors.New("any")},
		{s.mocked.ctxCanceled, "", errors.New("any"), "0.0.1", http.StatusOK, errors.New("any")},
		{s.mocked.ctxCanceled, "0.0.1", nil, "0.0.1", http.StatusOK, errors.New("any")},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		// http mock
		respLatest := httpmock.NewStringResponder(t.httpStatus, t.httpVer)

		urlAgent := fmt.Sprintf(`%v/yandexcloud-guestagent/release/%v/%v/%v/%v`,
			versionRemoteEndpoint, t.httpVer, runtime.GOOS, runtime.GOARCH, guest.AgentExecutable)
		respAgent := httpmock.NewStringResponder(t.httpStatus, "I R AGENT CONTENT")

		urlChecksum := joinWithDots(urlAgent, checksumSuffix)
		respChecksum := httpmock.NewStringResponder(t.httpStatus, "I R CHECKSUM")

		httpmock.RegisterResponder(http.MethodGet, guestAgentLatest, respLatest)
		httpmock.RegisterResponder(http.MethodGet, urlAgent, respAgent)
		httpmock.RegisterResponder(http.MethodGet, urlChecksum, respChecksum)
		httpmock.ActivateNonDefault(s.updater.hclient.GetClient())

		// repo mock
		r := mocks.Repository{}
		r.On("Get", t.httpVer).Return(t.repoGetRet)
		r.On("Add", mock.AnythingOfTypeArgument("string"), t.httpVer).Return(t.repoAddRet)

		// inject
		s.updater.fileRepo = &r
		s.updater.ctx = t.ctx

		err := s.updater.ensureLatestAdded()

		if t.wantErr != nil {
			s.Error(err)
		} else {
			s.NoError(err)
		}

		if t.ctx.Err() == nil && t.httpStatus == http.StatusOK {
			c := r.AssertCalled(s.T(), "Get", t.httpVer)
			s.True(c)

			if t.repoGetRet == "" {
				c = r.AssertCalled(s.T(), "Add", mock.AnythingOfTypeArgument("string"), t.httpVer)
				s.True(c)
			}
		}

		httpmock.DeactivateAndReset()
		s.updater.fs = afero.NewMemMapFs()
	}
}

func (s *updaterTests) TestMaybeAddInstalledVersion() {
	for n, t := range []struct {
		ctx     context.Context
		intFs   func(fs afero.Fs)
		retVer  string
		retErr  error
		retGet  string
		retAdd  error
		wantErr error
	}{
		{s.mocked.ctx, s.mocked.initAgentExistFS, "0.0.1", nil, "", nil, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "0.0.1", nil, "", errors.New("any"), errors.New("any")},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "0.0.1", nil, "any string", nil, nil},
		{s.mocked.ctx, s.mocked.initAgentExistFS, "", errors.New("any"), "", nil, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.initAgentNotExistFS, "0.0.1", nil, "", nil, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.initAgentNotExistFS, "0.0.1", nil, "", errors.New("any"), errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.initAgentNotExistFS, "0.0.1", nil, "any string", nil, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.initAgentNotExistFS, "", errors.New("any"), "", nil, errors.New("any")},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		t.intFs(s.updater.fs)
		s.updater.ctx = t.ctx
		s.updater.getAgentVersion = func(_ string) (string, error) {
			return t.retVer, t.retErr
		}

		f := mocks.Repository{}
		f.On("Get", t.retVer).Return(t.retGet)
		f.On("Add", filepath.Join(guest.AgentDir, guest.AgentExecutable), t.retVer).Return(t.retAdd)
		s.updater.fileRepo = &f

		err := s.updater.maybeAddInstalledVersion()

		if t.wantErr != nil {
			s.Error(err)
		} else {
			s.NoError(err)
		}

		s.updater.fs = afero.NewMemMapFs()
	}
}

func (s *updaterTests) TestEnsureService() {
	for n, t := range []struct {
		ctx     context.Context
		svcMgr  *mocks.ServiceManager
		wantErr error
	}{
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistNil, nil},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistErr, errors.New("any")},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistFalseOnCreateNil, nil},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistFalseOnCreateErr, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistNil, nil},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistErr, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistFalseOnCreateNil, nil},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistFalseOnCreateErr, errors.New("any")},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		s.updater.ctx = t.ctx
		s.updater.svcMgr = t.svcMgr

		err := s.updater.ensureService()

		if t.wantErr != nil {
			s.Error(err)
		} else {
			s.NoError(err)
		}

		t.svcMgr.AssertExpectations(s.T())
	}
}

func (s *updaterTests) TestEnsureNoService() {
	for n, t := range []struct {
		ctx     context.Context
		svcMgr  *mocks.ServiceManager
		wantErr error
	}{
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistFalse, nil},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistErr, errors.New("any")},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistTrueOnIsStoppedErr, errors.New("any")},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteNil, nil},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteErr, errors.New("any")},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistTrueOnIsStoppedFalseOnStopErr, errors.New("any")},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteErr, errors.New("any")},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteNil, nil},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistFalse, nil},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistErr, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistTrueOnIsStoppedErr, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteNil, nil},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteErr, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistTrueOnIsStoppedFalseOnStopErr, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteErr, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteNil, nil},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		s.updater.ctx = t.ctx
		s.updater.svcMgr = t.svcMgr

		err := s.updater.ensureNoService()

		if t.wantErr != nil {
			s.Error(err)
		} else {
			s.NoError(err)
		}

		t.svcMgr.AssertExpectations(s.T())
	}
}

func (s *updaterTests) TestEnsureDirectory() {
	for n, t := range []struct {
		fileOrDirCreator func(fs afero.Fs, src string)
		wantErr          error
	}{
		{func(_ afero.Fs, _ string) {}, nil},
		{s.createSpareDir, nil},
		{s.createSpareFile, errors.New("any")},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		t.fileOrDirCreator(s.updater.fs, guest.AgentDir)

		err := s.updater.ensureDirectory()
		if t.wantErr != nil {
			s.Error(err)
		} else {
			s.NoError(err)

			d, e := afero.IsDir(s.updater.fs, guest.AgentDir)
			s.NoError(e)
			s.True(d)
		}

		s.updater.fs = afero.NewMemMapFs()
	}
}

func (s *updaterTests) createSpareDir(fs afero.Fs, src string) {
	err := fs.MkdirAll(src, defaultPerms)
	s.Require().NoError(err)
}

func (s *updaterTests) TestEnsureNoDirectory() {
	s.createSpareDir(s.updater.fs, guest.AgentDir)

	err := s.updater.ensureNoDirectory()
	s.NoError(err)
}

func (s *updaterTests) createSpareFile(fs afero.Fs, src string) {
	err := afero.WriteFile(fs, src, []byte("I R FILE CONTENT!"), defaultPerms)
	s.Require().NoError(err)
}

func (s *updaterTests) TestEnsureCopy() {
	dst := "/copied/to.txt"
	src := "/copy/me/from.txt"
	s.createSpareFile(s.updater.fs, src)

	// test
	s.NoError(s.updater.ensureCopy(dst, src))
	exist, err := afero.Exists(s.updater.fs, dst)
	s.NoError(err)
	s.True(exist)
}

func (s *updaterTests) TestRemove() {
	for n, t := range []struct {
		ctx     context.Context
		svcMgr  *mocks.ServiceManager
		wantErr error
	}{
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistFalse, nil},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistErr, errors.New("any")},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistTrueOnIsStoppedErr, errors.New("any")},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteNil, nil},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteErr, errors.New("any")},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistTrueOnIsStoppedFalseOnStopErr, errors.New("any")},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteErr, errors.New("any")},
		{s.mocked.ctx, s.mocked.svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteNil, nil},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistFalse, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistErr, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistTrueOnIsStoppedErr, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteNil, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistTrueOnIsStoppedTrueOnDeleteErr, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistTrueOnIsStoppedFalseOnStopErr, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteErr, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnIsExistTrueOnIsStoppedFalseOnStopNilOnDeleteNil, errors.New("any")},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		err := s.updater.fs.Mkdir(guest.AgentDir, defaultPerms)
		s.Require().NoError(err)

		s.updater.ctx = t.ctx
		s.updater.svcMgr = t.svcMgr

		err = s.updater.Remove()

		if t.wantErr != nil {
			s.Error(err)
		} else {
			s.NoError(err)

			exist, exErr := afero.Exists(s.updater.fs, filepath.Join(guest.AgentDir, guest.AgentExecutable))
			s.False(exist)
			s.NoError(exErr)
		}

		s.updater.fs = afero.NewMemMapFs()
		t.svcMgr.AssertExpectations(s.T())
	}
}

func (s *updaterTests) TestStart() {
	for n, t := range []struct {
		ctx     context.Context
		svc     *mocks.ServiceManager
		wantErr error
	}{
		{s.mocked.ctx, s.mocked.svcMgrOnStartNil, nil},
		{s.mocked.ctx, s.mocked.svcMgrOnStartErr, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnStartNil, nil},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnStartErr, errors.New("any")},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		s.updater.ctx = t.ctx
		s.updater.svcMgr = t.svc
		err := s.updater.Start()

		if t.wantErr != nil {
			s.Error(err)
		} else {
			s.NoError(err)
		}

		t.svc.AssertExpectations(s.T())
	}
}

func (s *updaterTests) TestStop() {
	for n, t := range []struct {
		ctx     context.Context
		svc     *mocks.ServiceManager
		wantErr error
	}{
		{s.mocked.ctx, s.mocked.svcMgrOnStopNil, nil},
		{s.mocked.ctx, s.mocked.svcMgrOnStopErr, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnStopNil, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnStopErr, errors.New("any")},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		s.updater.ctx = t.ctx
		s.updater.svcMgr = t.svc
		err := s.updater.Stop()

		if t.wantErr != nil {
			s.Error(err)
		} else {
			s.NoError(err)
		}

		t.svc.AssertExpectations(s.T())
	}
}

func (s *updaterTests) TestClose() {
	for n, t := range []struct {
		ctx     context.Context
		svc     *mocks.ServiceManager
		wantErr error
	}{
		{s.mocked.ctx, s.mocked.svcMgrOnCloseNil, nil},
		{s.mocked.ctx, s.mocked.svcMgrOnCloseErr, errors.New("any")},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnCloseNil, nil},
		{s.mocked.ctxCanceled, s.mocked.svcMgrOnCloseErr, errors.New("any")},
	} {
		logger.DebugCtx(s.updater.ctx, nil, "----------running test----------", zap.Int("number", n))

		s.updater.ctx = t.ctx
		s.updater.svcMgr = t.svc
		err := s.updater.Close()

		if t.wantErr != nil {
			s.Error(err)
		} else {
			s.NoError(err)
		}

		t.svc.AssertExpectations(s.T())
	}
}

//
// helpers
//

type joinWithDotsTableTests struct{ suite.Suite }

func (s *joinWithDotsTableTests) TestJoinWithDots() {
	s.Require().Equal("one.by.one", joinWithDots("one", "by", "one"))
	s.Require().Equal("..", joinWithDots("", "", ""))
	s.Require().Equal("", joinWithDots(""))
}
