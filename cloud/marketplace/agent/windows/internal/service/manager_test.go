package service

import (
	"context"
	"errors"
	"path/filepath"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zaptest"
	"golang.org/x/sys/windows/svc"
	"golang.org/x/sys/windows/svc/mgr"

	"a.yandex-team.ru/cloud/marketplace/agent/pkg/logger"
	"a.yandex-team.ru/cloud/marketplace/agent/windows/internal/guest"
	"a.yandex-team.ru/cloud/marketplace/agent/windows/internal/service/mocks"
)

func TestService(t *testing.T) {
	suite.Run(t, new(serviceTests))
}

type serviceTests struct {
	suite.Suite
	m    *Manager
	mock mocked
}

type mocked struct {
	svcName                    string
	err                        error
	svcDummy                   *mocks.Service
	svcError                   *mocks.Service
	svcDelete                  *mocks.Service
	svcDeleteFail              *mocks.Service
	svcRunning                 *mocks.Service
	svcStopped                 *mocks.Service
	svcStoppedToRunning        *mocks.Service
	svcStoppedToRunningFail    *mocks.Service
	svcStoppedToRunningTimeout *mocks.Service
	svcRunningToStopped        *mocks.Service
	svcRunningToStoppedFail    *mocks.Service
	svcRunningToStoppedTimeout *mocks.Service
	mgrSvcExist                *mocks.Manager
	mgrSvcNotExist             *mocks.Manager
	mockMgrSvcCreate           *mocks.Manager
	mockMgrSvcCreateFail       *mocks.Manager
}

func (s *serviceTests) SetupTest() {
	l := zaptest.NewLogger(s.T())
	ctx := logger.NewContext(context.Background(), l)

	var err error
	s.m, err = NewManager(ctx)

	s.Require().NotNil(s.m)
	s.Require().NoError(err)

	s.mock = initMocks()

	// it is a global variable, exposed as const, use override it to speedup some tests
	timeout = 2 * time.Second
}

func initMocks() mocked {
	serviceName := "myDummyServiceName"

	mockErr := errors.New("to error, or not to error")

	mockMgrSvcExist := mocks.Manager{}
	mockMgrSvcExist.On("ListServices").Return([]string{serviceName}, nil)

	mockMgrSvcNotExist := mocks.Manager{}
	mockMgrSvcNotExist.On("ListServices").Return([]string{""}, nil)

	mockMgrSvcCreate := mocks.Manager{}
	mockMgrSvcCreate.On("ListServices").Return([]string{""}, nil)
	mockMgrSvcCreate.On("CreateService",
		serviceName,
		filepath.Join(guest.AgentDir, guest.AgentExecutable),
		mgr.Config{
			DisplayName: guest.ServiceDescription,
			Description: guest.ServiceDescription,
			StartType:   mgr.StartAutomatic,
		}).Return(&mgr.Service{}, nil)

	mockMgrSvcCreateFail := mocks.Manager{}
	mockMgrSvcCreateFail.On("ListServices").Return([]string{""}, nil)
	mockMgrSvcCreateFail.On("CreateService",
		serviceName,
		filepath.Join(guest.AgentDir, guest.AgentExecutable),
		mgr.Config{
			DisplayName: guest.ServiceDescription,
			Description: guest.ServiceDescription,
			StartType:   mgr.StartAutomatic,
		}).Return(nil, mockErr)

	// to assert no expectations
	mockSvcDummy := mocks.Service{}

	mockSvcRunning := mocks.Service{}
	mockSvcRunning.On("Query").Return(svc.Status{State: svc.Running}, nil)
	mockSvcRunning.On("Close").Return(nil)

	mockSvcError := mocks.Service{}
	mockSvcError.On("Query").Return(svc.Status{State: svc.State(Unknown)}, mockErr)
	mockSvcError.On("Close").Return(nil)

	mockSvcStopped := mocks.Service{}
	mockSvcStopped.On("Query").Return(svc.Status{State: svc.Stopped}, nil)
	mockSvcStopped.On("Close").Return(nil)

	mockSvcStoppedToRunning := mocks.Service{}
	mockSvcStoppedToRunning.On("Query").Once().Return(svc.Status{State: svc.Stopped}, nil)
	mockSvcStoppedToRunning.On("Query").Return(svc.Status{State: svc.Running}, nil)
	mockSvcStoppedToRunning.On("Start").Return(nil)
	mockSvcStoppedToRunning.On("Close").Return(nil)

	mockSvcStoppedToRunningTimeout := mocks.Service{}
	mockSvcStoppedToRunningTimeout.On("Query").Return(svc.Status{State: svc.Stopped}, nil)
	mockSvcStoppedToRunningTimeout.On("Start").Return(nil)
	mockSvcStoppedToRunningTimeout.On("Close").Return(nil)

	mockSvcStoppedToRunningFail := mocks.Service{}
	mockSvcStoppedToRunningFail.On("Query").Return(svc.Status{State: svc.Stopped}, nil)
	mockSvcStoppedToRunningFail.On("Start").Return(mockErr)
	mockSvcStoppedToRunningFail.On("Close").Return(nil)

	mockSvcRunningToStopped := mocks.Service{}
	mockSvcRunningToStopped.On("Query").Once().Return(svc.Status{State: svc.Running}, nil)
	mockSvcRunningToStopped.On("Query").Return(svc.Status{State: svc.Stopped}, nil)
	mockSvcRunningToStopped.On("Control", svc.Stop).Return(svc.Status{}, nil)
	mockSvcRunningToStopped.On("Close").Return(nil)

	mockSvcRunningToStoppedTimeout := mocks.Service{}
	mockSvcRunningToStoppedTimeout.On("Query").Return(svc.Status{State: svc.Running}, nil)
	mockSvcRunningToStoppedTimeout.On("Control", svc.Stop).Return(svc.Status{}, nil)
	mockSvcRunningToStoppedTimeout.On("Close").Return(nil)

	mockSvcRunningToStoppedFail := mocks.Service{}
	mockSvcRunningToStoppedFail.On("Query").Return(svc.Status{State: svc.Running}, nil)
	mockSvcRunningToStoppedFail.On("Control", svc.Stop).Return(svc.Status{}, mockErr)
	mockSvcRunningToStoppedFail.On("Close").Return(nil)

	mockSvcDelete := mocks.Service{}
	mockSvcDelete.On("Delete").Return(nil)
	mockSvcDelete.On("Close").Return(nil)

	mockSvcDeleteFail := mocks.Service{}
	mockSvcDeleteFail.On("Delete").Return(mockErr)
	mockSvcDeleteFail.On("Close").Return(nil)

	return mocked{
		svcName:                    serviceName,
		err:                        mockErr,
		svcError:                   &mockSvcError,
		svcDummy:                   &mockSvcDummy,
		svcDelete:                  &mockSvcDelete,
		svcDeleteFail:              &mockSvcDeleteFail,
		svcRunning:                 &mockSvcRunning,
		svcStopped:                 &mockSvcStopped,
		svcStoppedToRunning:        &mockSvcStoppedToRunning,
		svcStoppedToRunningFail:    &mockSvcStoppedToRunningFail,
		svcStoppedToRunningTimeout: &mockSvcStoppedToRunningTimeout,
		svcRunningToStopped:        &mockSvcRunningToStopped,
		svcRunningToStoppedFail:    &mockSvcRunningToStoppedFail,
		svcRunningToStoppedTimeout: &mockSvcRunningToStoppedTimeout,
		mgrSvcExist:                &mockMgrSvcExist,
		mgrSvcNotExist:             &mockMgrSvcNotExist,
		mockMgrSvcCreate:           &mockMgrSvcCreate,
		mockMgrSvcCreateFail:       &mockMgrSvcCreateFail,
	}
}

func (s *serviceTests) TestNewManager() {
	m, err := NewManager(context.Background())

	s.NotNil(m)
	s.NoError(err)
}

//nolint:SA1012
func (s *serviceTests) TestNewManagerFailOnNilCtx() {
	m, err := NewManager(nil)

	s.Nil(m)
	s.Error(err)

}

func (s *serviceTests) TestCloseUnit() {
	tests := []struct {
		retErr error
	}{
		{nil},
		{s.mock.err},
	}

	for _, t := range tests {
		m := mocks.Manager{}
		m.On("Disconnect").Return(t.retErr)
		s.m.mgr = &m

		if t.retErr != nil {
			s.ErrorIs(s.m.Close(), t.retErr)
		} else {
			s.Nil(s.m.Close())
		}

		m.AssertCalled(s.T(), "Disconnect")
	}
}

func (s *serviceTests) TestIsExistUnit() {
	mockErr := errors.New("to error, or not to error")
	tests := []struct {
		retVal []string
		retErr error
		want   bool
	}{
		{[]string{""}, mockErr, false},
		{[]string{""}, nil, false},
		{[]string{"", "some", "services", "names"}, mockErr, false},
		{[]string{"", "some", "services", "names"}, nil, false},
		{[]string{s.mock.svcName}, mockErr, false},
		{[]string{s.mock.svcName}, nil, true},
		{[]string{"", "but", "who", s.mock.svcName, "cares"}, mockErr, false},
		{[]string{"", "but", "who", s.mock.svcName, "cares"}, nil, true},
	}

	for _, t := range tests {
		m := mocks.Manager{}
		m.On("ListServices").Return(t.retVal, t.retErr)
		s.m.mgr = &m

		// test
		exist, err := s.m.IsExist(s.mock.svcName)

		if t.want {
			s.True(exist)
		} else {
			s.False(exist)
		}

		if t.retErr != nil {
			s.ErrorIs(err, t.retErr)
		} else {
			s.Nil(err)
		}

		m.AssertCalled(s.T(), "ListServices")
	}
}

func (s *serviceTests) TestIsStopped() {
	tests := []struct {
		mgr       *mocks.Manager
		openerSvc *mocks.Service
		openerErr error
		wantRes   bool
		wantErr   error
	}{
		{s.mock.mgrSvcExist, s.mock.svcRunning, nil, false, nil},
		{s.mock.mgrSvcExist, s.mock.svcStopped, nil, true, nil},
		{s.mock.mgrSvcExist, s.mock.svcRunning, s.mock.err, false, s.mock.err},
		{s.mock.mgrSvcExist, s.mock.svcStopped, s.mock.err, false, s.mock.err},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, nil, false, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, nil, false, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, s.mock.err, false, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, s.mock.err, false, ErrNotFound},
	}

	for _, t := range tests {
		s.m.mgr = t.mgr
		s.m.openService = func(_ string) (service, error) {
			return t.openerSvc, t.openerErr
		}

		stopped, err := s.m.IsStopped(s.mock.svcName)

		if t.wantErr != nil {
			s.ErrorIs(err, t.wantErr)
		} else {
			s.Nil(err)
		}

		if t.wantRes {
			s.True(stopped)
		} else {
			s.False(stopped)
		}

		t.mgr.AssertExpectations(s.T())
		t.openerSvc.AssertExpectations(s.T())
	}
}

func (s *serviceTests) TestIsRunning() {
	tests := []struct {
		mgr       *mocks.Manager
		openerSvc *mocks.Service
		openerErr error
		wantRes   bool
		wantErr   error
	}{
		{s.mock.mgrSvcExist, s.mock.svcRunning, nil, true, nil},
		{s.mock.mgrSvcExist, s.mock.svcStopped, nil, false, nil},
		{s.mock.mgrSvcExist, s.mock.svcRunning, s.mock.err, false, s.mock.err},
		{s.mock.mgrSvcExist, s.mock.svcStopped, s.mock.err, false, s.mock.err},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, nil, false, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, nil, false, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, s.mock.err, false, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, s.mock.err, false, ErrNotFound},
	}

	for _, t := range tests {
		s.m.mgr = t.mgr
		s.m.openService = func(_ string) (service, error) {
			return t.openerSvc, t.openerErr
		}

		running, err := s.m.IsRunning(s.mock.svcName)

		if t.wantErr != nil {
			s.ErrorIs(err, t.wantErr)
		} else {
			s.Nil(err)
		}

		if t.wantRes {
			s.True(running)
		} else {
			s.False(running)
		}

		t.mgr.AssertExpectations(s.T())
		t.openerSvc.AssertExpectations(s.T())
	}
}

func (s *serviceTests) TestGetStatus() {
	tests := []struct {
		mgr       *mocks.Manager
		openerSvc *mocks.Service
		openerErr error
		wantRes   State
		wantErr   error
	}{
		{s.mock.mgrSvcExist, s.mock.svcRunning, nil, Running, nil},
		{s.mock.mgrSvcExist, s.mock.svcStopped, nil, Stopped, nil},
		{s.mock.mgrSvcExist, s.mock.svcRunning, s.mock.err, Unknown, s.mock.err},
		{s.mock.mgrSvcExist, s.mock.svcStopped, s.mock.err, Unknown, s.mock.err},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, nil, Unknown, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, nil, Unknown, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, s.mock.err, Unknown, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, s.mock.err, Unknown, ErrNotFound},
	}

	for _, t := range tests {
		s.m.mgr = t.mgr
		s.m.openService = func(_ string) (service, error) {
			return t.openerSvc, t.openerErr
		}

		state, err := s.m.getStatus(s.mock.svcName)

		if t.wantErr != nil {
			s.ErrorIs(err, t.wantErr)
		} else {
			s.Nil(err)
		}

		s.Equal(t.wantRes, state)
		t.mgr.AssertExpectations(s.T())
		t.openerSvc.AssertExpectations(s.T())
	}
}

func (s *serviceTests) TestStart() {
	tests := []struct {
		mgr       *mocks.Manager
		openerSvc *mocks.Service
		openerErr error
		wantErr   error
	}{
		{s.mock.mgrSvcExist, s.mock.svcStoppedToRunning, nil, nil},
		{s.mock.mgrSvcExist, s.mock.svcStoppedToRunningFail, nil, s.mock.err},
		{s.mock.mgrSvcExist, s.mock.svcStoppedToRunningTimeout, nil, ErrTimeout},
		{s.mock.mgrSvcExist, s.mock.svcDummy, s.mock.err, s.mock.err},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, nil, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, nil, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, nil, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, nil, ErrNotFound},
	}

	for _, t := range tests {
		s.m.mgr = t.mgr
		s.m.openService = func(_ string) (service, error) {
			return t.openerSvc, t.openerErr
		}

		err := s.m.Start(s.mock.svcName)

		if t.wantErr != nil {
			s.ErrorIs(err, t.wantErr)
		} else {
			s.Nil(err)
		}

		t.mgr.AssertExpectations(s.T())
		t.openerSvc.AssertExpectations(s.T())
	}
}

func (s *serviceTests) TestStop() {
	tests := []struct {
		mgr       *mocks.Manager
		openerSvc *mocks.Service
		openerErr error
		wantErr   error
	}{
		{s.mock.mgrSvcExist, s.mock.svcRunningToStopped, nil, nil},
		{s.mock.mgrSvcExist, s.mock.svcRunningToStoppedFail, nil, s.mock.err},
		{s.mock.mgrSvcExist, s.mock.svcRunningToStoppedTimeout, nil, ErrTimeout},
		{s.mock.mgrSvcExist, s.mock.svcDummy, s.mock.err, s.mock.err},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, nil, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, nil, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, nil, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, nil, ErrNotFound},
	}

	for _, t := range tests {
		s.m.mgr = t.mgr
		s.m.openService = func(_ string) (service, error) {
			return t.openerSvc, t.openerErr
		}

		err := s.m.Stop(s.mock.svcName)

		if t.wantErr != nil {
			s.ErrorIs(err, t.wantErr)
		} else {
			s.Nil(err)
		}

		t.mgr.AssertExpectations(s.T())
		t.openerSvc.AssertExpectations(s.T())
	}
}
func (s *serviceTests) TestCreateService() {
	tests := []struct {
		mgr     *mocks.Manager
		wantErr error
	}{
		{s.mock.mockMgrSvcCreate, nil},
		{s.mock.mockMgrSvcCreateFail, s.mock.err},
		{s.mock.mgrSvcExist, ErrAlreadyExist},
	}

	for _, t := range tests {
		s.m.mgr = t.mgr
		err := s.m.Create(filepath.Join(guest.AgentDir, guest.AgentExecutable),
			s.mock.svcName,
			guest.ServiceDescription,
			guest.ServiceDescription)

		if t.wantErr != nil {
			s.ErrorIs(err, t.wantErr)
		} else {
			s.Nil(err)
		}

		t.mgr.AssertExpectations(s.T())
	}
}

func (s *serviceTests) TestDelete() {
	tests := []struct {
		mgr       *mocks.Manager
		openerSvc *mocks.Service
		openerErr error
		wantErr   error
	}{
		{s.mock.mgrSvcExist, s.mock.svcDelete, nil, nil},
		{s.mock.mgrSvcExist, s.mock.svcDelete, s.mock.err, s.mock.err},
		{s.mock.mgrSvcExist, s.mock.svcDeleteFail, nil, s.mock.err},
		{s.mock.mgrSvcExist, s.mock.svcDeleteFail, s.mock.err, s.mock.err},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, nil, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, s.mock.err, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, nil, ErrNotFound},
		{s.mock.mgrSvcNotExist, s.mock.svcDummy, s.mock.err, ErrNotFound},
	}

	for _, t := range tests {
		s.m.mgr = t.mgr
		s.m.openService = func(_ string) (service, error) {
			return t.openerSvc, t.openerErr
		}

		err := s.m.Delete(s.mock.svcName)

		if t.wantErr != nil {
			s.ErrorIs(err, t.wantErr)
		} else {
			s.Nil(err)
		}

		t.mgr.AssertExpectations(s.T())
		t.openerSvc.AssertExpectations(s.T())
	}
}
