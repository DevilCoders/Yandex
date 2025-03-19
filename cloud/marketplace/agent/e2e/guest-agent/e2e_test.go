package e2e

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/gruntwork-io/terratest/modules/logger"
	"github.com/gruntwork-io/terratest/modules/terraform"
	"github.com/stretchr/testify/suite"
	ycsdk "github.com/yandex-cloud/go-sdk"
)

func TestPasswordResetPipeline(t *testing.T) {
	s := new(passwordResetPipeline)
	suite.Run(t, s)
}

type passwordResetPipeline struct {
	suite.Suite
	cfg              *config
	terraformOptions *terraform.Options
	windowsInstance  *windowsInstance
	psAdm            winrs
	psUsr            winrs
}

const (
	testUsername  = "kenny"
	adminUsername = "Administrator"
)

// SetupSuite function creates instance and uploads agent.
func (s *passwordResetPipeline) SetupSuite() {
	var err error
	s.cfg, err = loadConfig()
	s.NotEmpty(s.cfg)
	s.NoError(err)

	// create network, subnet, security group and instance with bootstrap script
	s.terraformOptions = terraform.WithDefaultRetryableErrors(s.T(), &terraform.Options{
		TerraformDir: "./terraform",
		Vars: map[string]interface{}{
			"token":      s.cfg.Token,
			"cloud_id":   s.cfg.CloudID,
			"folder_id":  s.cfg.FolderID,
			"admin_pass": s.cfg.InstancePWD,
		},
		NoColor: true,
	})

	_, err = terraform.InitAndApplyE(s.T(), s.terraformOptions)
	s.NoError(err)

	ctx, cancel := context.WithTimeout(context.Background(), ycTimeout)
	defer cancel()
	var yc *ycsdk.SDK
	yc, err = newYCClient(ctx, s.cfg.Token)
	s.NoError(err)
	s.NotEmpty(yc)

	var instanceID string
	instanceID, err = terraform.OutputE(s.T(), s.terraformOptions, "instance_id")
	s.NoError(err)
	s.NotEmpty(instanceID)

	var i *computeInstance
	i, err = lookupComputeInstance(yc, instanceID)
	s.NotEmpty(i)
	s.NoError(err)
	s.windowsInstance = newWindowsInstance(i)

	instanceAddress := terraform.Output(s.T(), s.terraformOptions, "address")
	s.NotEmpty(instanceAddress)

	s.psUsr = newWinRS(testUsername, "", instanceAddress)
	logger.Log(s.T(), "user config:", s.psUsr)

	s.psAdm = newWinRS(adminUsername, s.cfg.InstancePWD, instanceAddress)
	logger.Log(s.T(), "admin config:", s.psAdm)

	logger.Log(s.T(), "upload:", s.cfg.Source, "to", s.cfg.Destination, "this could take up to 15m")
	s.FileExists(s.cfg.Source)
	s.NoError(s.psAdm.uploadFile(s.cfg.Destination, s.cfg.Source))
}

func (s *passwordResetPipeline) TearDownSuite() {
	_, err := terraform.DestroyE(s.T(), s.terraformOptions)
	s.NoError(err)
}

// windowsKeyRegPath path to registry key, that hold agent configuration.
const windowsKeyRegPath = `HKLM:\SOFTWARE\Yandex\Cloud\Compute`

// TearDownTest terminates agent process, resets registry key and metadata field.
func (s *passwordResetPipeline) TearDownTest() {
	// remove registry keys
	logger.Log(s.T(), "remove agent registry key")
	err := s.psAdm.removeRegKey(windowsKeyRegPath)
	s.NoError(err)

	// remove metadata
	logger.Log(s.T(), "remove 'windows-users' metadata field")
	err = s.windowsInstance.updateMetadata(windowsUsersMetadataKey, "")
	s.NoError(err)

	// delete user
	logger.Log(s.T(), "remove user")
	err = s.psAdm.removeUser(testUsername)
	s.NoError(err)
}

// remoteMgmtUsers supplementary group to which we add created user to logon via winrm.
const remoteMgmtUsers = "Remote Management Users"

// TestCreateUser resets password for non-existent user thereby creating him.
func (s *passwordResetPipeline) TestCreateUser() {
	logger.Log(s.T(), "start agent")
	pid, err := s.psAdm.startProcess(s.cfg.Destination, "start --log-level Debug")
	s.NoError(err)
	s.NotZero(pid)

	logger.Log(s.T(), "remote agent process id:", pid)
	s.NoError(s.psAdm.checkProcess(pid))

	logger.Log(s.T(), "create encryption key")
	var encKey *encryptionKey
	encKey, err = newEncryptionKey()
	s.NoError(err)
	s.NotEmpty(encKey)

	logger.Log(s.T(), "create user change request")
	req := newWindowsUserChangeRequest(testUsername, encKey.GetModulus(), encKey.GetExponent())
	logger.Log(s.T(), "user change request:", fmt.Sprintf("%+v", req))

	logger.Log(s.T(), "change windows user")
	resp, err := s.windowsInstance.updateWindowsUsers(req)
	s.NoError(err)
	s.Equal(true, resp.Success)
	logger.Log(s.T(), "parsed user change response:", fmt.Sprintf("%+v", resp))

	logger.Log(s.T(), "decrypt password")
	var password string
	password, err = encKey.Decrypt(resp.EncryptedPassword)
	s.NoError(err)
	s.NotEqual("", password)
	logger.Log(s.T(), "decrypted password:", password)

	// give testing user permissions for remote psrp connection
	logger.Log(s.T(), "add", testUsername, "to group", remoteMgmtUsers, "for winrm test")
	s.NoError(s.psAdm.addLocalGroupMember(remoteMgmtUsers, testUsername))

	// test connection
	s.psUsr.password = password
	logger.Log(s.T(), testUsername, "winrm config:", s.psUsr)
	logger.Log(s.T(), "connect to instance via windows remote shell")
	s.NoError(s.psUsr.whoami())

	// check group membership
	var ok bool
	ok, err = s.psAdm.checkMemberOfAdministrators(testUsername)
	s.NoError(err)
	s.True(ok)

	logger.Log(s.T(), "stop agent process")
	s.NoError(s.psAdm.stopProcess(pid))
}

// TestSameRequest sends same request to test idempotency.
func (s *passwordResetPipeline) TestSameRequest() {
	logger.Log(s.T(), "start agent")
	pid, err := s.psAdm.startProcess(s.cfg.Destination, "start --log-level Debug")
	s.NoError(err)
	s.NotZero(pid)

	logger.Log(s.T(), "remote agent process id:", pid)
	s.NoError(s.psAdm.checkProcess(pid))

	logger.Log(s.T(), "create encryption key")
	encKey, err := newEncryptionKey()
	s.NoError(err)
	s.NotEmpty(encKey)

	logger.Log(s.T(), "create user change request")
	req := newWindowsUserChangeRequest(testUsername, encKey.GetModulus(), encKey.GetExponent())
	logger.Log(s.T(), "user change request:", fmt.Sprintf("%+v", req))

	logger.Log(s.T(), "change windows user")
	resp, err := s.windowsInstance.updateWindowsUsers(req)
	s.NoError(err)
	s.Equal(true, resp.Success)
	logger.Log(s.T(), "parsed user change response:", fmt.Sprintf("%+v", resp))

	logger.Log(s.T(), "send same windows user change request")
	resp, err = s.windowsInstance.updateWindowsUsers(req)
	s.EqualError(err, context.DeadlineExceeded.Error())
	s.Equal(false, resp.Success)
	logger.Log(s.T(), "parsed user change response:", fmt.Sprintf("%+v", resp))

	logger.Log(s.T(), "stop agent process")
	s.NoError(s.psAdm.stopProcess(pid))
}

// TestResetExistingUser test password reset for existing user.
func (s *passwordResetPipeline) TestResetExistingUser() {
	logger.Log(s.T(), "start agent")
	pid, err := s.psAdm.startProcess(s.cfg.Destination, "start --log-level Debug")
	s.NoError(err)
	s.NotZero(pid)

	// we use existing password
	logger.Log(s.T(), "create user via powershell")
	s.NoError(s.psAdm.createAdministrator(testUsername, s.cfg.InstancePWD))

	logger.Log(s.T(), "create encryption key")
	encKey, err := newEncryptionKey()
	s.NoError(err)
	s.NotEmpty(encKey)

	logger.Log(s.T(), "create user change request")
	req := newWindowsUserChangeRequest(testUsername, encKey.GetModulus(), encKey.GetExponent())
	logger.Log(s.T(), "user change request:", fmt.Sprintf("%+v", req))

	logger.Log(s.T(), "change windows user")
	resp, err := s.windowsInstance.updateWindowsUsers(req)
	s.NoError(err)
	s.Equal(true, resp.Success)
	logger.Log(s.T(), "parsed user change response:", fmt.Sprintf("%+v", resp))

	logger.Log(s.T(), "decrypt password")
	password, err := encKey.Decrypt(resp.EncryptedPassword)
	s.NoError(err)
	s.NotEqual("", password)
	logger.Log(s.T(), "decrypted password:", password)

	// test connection
	s.psUsr.password = password
	logger.Log(s.T(), testUsername, "winrm config:", s.psUsr)
	logger.Log(s.T(), "connect to instance via windows remote shell")
	s.NoError(s.psUsr.whoami())

	logger.Log(s.T(), "stop agent process")
	s.NoError(s.psAdm.stopProcess(pid))
}

// TestHeartbeat starts agent and waits 2m to check if any heartbeats sent.
func (s *passwordResetPipeline) TestHeartbeat() {
	logger.Log(s.T(), "start agent")
	pid, err := s.psAdm.startProcess(s.cfg.Destination, "start --log-level Debug")
	s.NoError(err)
	s.NotZero(pid)

	<-time.After(120 * time.Second)
	s.NoError(s.windowsInstance.checkHeartbeat())

	logger.Log(s.T(), "stop agent process")
	s.NoError(s.psAdm.stopProcess(pid))
}

const arbitraryWait = 10 * time.Second

// TestSendSIGINT checks if agent process detects console SIGINT signal and gracefully stop.
func (s *passwordResetPipeline) TestSendSIGINT() {
	logger.Log(s.T(), "test Ctrl+C reaction")
	s.NoError(s.psAdm.startAndStopByCtrlC(agentDest), "send CtrlC")
	<-time.After(arbitraryWait)
	s.NoError(s.windowsInstance.checkTermMessages())
}

// TestSerialLog checks if log messages arrive at serial port.
func (s *passwordResetPipeline) TestSerialLog() {
	logger.Log(s.T(), "start agent")
	pid, err := s.psAdm.startProcess(s.cfg.Destination, "start --log-level Debug")
	s.NoError(err)
	s.NotZero(pid)

	logger.Log(s.T(), "remote agent process id:", pid)
	s.NoError(s.psAdm.checkProcess(pid))

	<-time.After(arbitraryWait)
	s.NoError(s.windowsInstance.checkLogMessages())

	logger.Log(s.T(), "stop agent process")
	s.NoError(s.psAdm.stopProcess(pid))
}

const serviceName = "yc-guest-agent"

// TestInstallStartStopUninstallService checks if agent could be installed/started/stopped/uninstalled as a service.
func (s *passwordResetPipeline) TestInstallStartStopUninstallService() {
	logger.Log(s.T(), "install agent")
	_, err := s.psAdm.startProcess(s.cfg.Destination, "install")
	s.NoError(err)

	<-time.After(arbitraryWait)

	s.NoError(s.psAdm.getService(serviceName))
	s.NoError(s.psAdm.startService(serviceName))
	s.NoError(s.psAdm.checkService(serviceName))
	s.NoError(s.psAdm.stopService(serviceName))

	_, err = s.psAdm.startProcess(s.cfg.Destination, "uninstall")
	s.NoError(err)
	s.Error(s.psAdm.getService(serviceName))
}

const agentProcessName = "agent"

// TestFailToStartSecondAgentInstance checks if start of second agent instance fails.
func (s *passwordResetPipeline) TestFailToStartSecondAgentInstance() {
	pid, err := s.psAdm.startProcess(s.cfg.Destination, "start --log-level Debug")
	logger.Log(s.T(), "start first instance of agent, pid:", pid)
	s.NoError(err)

	<-time.After(arbitraryWait)

	// start second agent
	var p int
	p, err = s.psAdm.startProcess(s.cfg.Destination, "start --log-level Debug")
	logger.Log(s.T(), "start second instance of agent, pid:", p)
	s.NoError(err)

	// check only first agent instance running
	p, err = s.psAdm.getProcessID(agentProcessName)
	logger.Log(s.T(), "received agent pid: ", p)
	s.NoError(err)
	s.Equal(pid, p)
	s.NoError(s.psAdm.stopProcess(pid), "stop agent process")
}
