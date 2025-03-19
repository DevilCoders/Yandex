package e2eguestagentupdater

import (
	"context"
	"crypto/sha256"
	"fmt"
	"io"
	"os"
	"strings"
	"testing"
	"time"

	"github.com/go-resty/resty/v2"
	"github.com/gruntwork-io/terratest/modules/terraform"
	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
	"go.uber.org/zap"
	"go.uber.org/zap/zaptest"

	"a.yandex-team.ru/cloud/marketplace/agent/e2e/winrs"
	"a.yandex-team.ru/cloud/marketplace/agent/pkg/logger"
	"a.yandex-team.ru/cloud/marketplace/agent/pkg/passwords"
)

const terraformDir = "./terraform"

// check if resources already created => noop

func TestGuestAgentUpdater(t *testing.T) {
	ctx := logger.NewContext(context.Background(), zaptest.NewLogger(t))

	conf, err := loadConfig()
	require.NoError(t, err)
	logger.DebugCtx(ctx, nil, "config loaded",
		zap.String("token", "**********"+conf.Token[10:len(conf.Token)-10]+"**********"),
		zap.String("cloud-id", conf.CloudID),
		zap.String("folder-id", conf.FolderID))
	require.NoError(t, err)

	terraformOptions := terraform.WithDefaultRetryableErrors(t,
		&terraform.Options{
			TerraformDir: terraformDir,
			EnvVars: map[string]string{
				"TF_VAR_cloud_id":  conf.CloudID,
				"TF_VAR_folder_id": conf.FolderID,
				"TF_VAR_token":     conf.Token,
			},
			NoColor: true,
		})

	out := terraform.Init(t, terraformOptions)
	logger.DebugCtx(ctx, nil, "terraform init",
		zap.String("stdout & stderr", out))

	if _, exist := os.LookupEnv("E2E_NO_DESTROY"); !exist {
		t.Cleanup(func() {
			out, _ := terraform.DestroyE(t, terraformOptions)
			logger.DebugCtx(ctx, nil, "terraform destroy",
				zap.String("stdout & stderr", out))
		})
	}

	plannedChanges := terraform.GetResourceCount(t, terraform.Plan(t, terraformOptions))
	logger.DebugCtx(ctx, nil, "planned changes",
		zap.Int("add", plannedChanges.Add),
		zap.Int("change", plannedChanges.Change),
		zap.Int("destroy", plannedChanges.Destroy))
	if plannedChanges.Add > 0 {
		const useDefaults = ""
		const length = uint(15)
		const numDigits = uint(3)
		const numSymbols = uint(5)
		const noUppers = false

		pwd, err := passwords.NewGenerator(useDefaults, useDefaults, useDefaults, useDefaults).
			Generate(length, numDigits, numSymbols, noUppers)
		logger.DebugCtx(ctx, err, "generate password",
			zap.String("pwd", "***"+pwd[3:len(pwd)-3]+"***"))
		require.NoError(t, err)

		terraformOptions.EnvVars["TF_VAR_admin_pass"] = pwd

		out := terraform.Apply(t, terraformOptions)
		logger.DebugCtx(ctx, nil, "terraform init and apply",
			zap.String("stdout & stderr", out))
	}

	suite.Run(t, new(guestAgentUpdaterSuite))
}

type guestAgentUpdaterSuite struct {
	ctx  context.Context
	psrp *winrs.WinRS
	suite.Suite
}

const updater = `guest-agent-updater`
const updaterFilename = updater + `.exe`
const updaterAtTemp = `C:\Windows\Temp\` + updaterFilename

func (s *guestAgentUpdaterSuite) SetupSuite() {
	s.ctx = logger.NewContext(context.Background(), zaptest.NewLogger(s.T()))

	logger.DebugCtx(s.ctx, nil, "setup suite",
		zap.String("terraform dir", terraformDir))
	terraformOptions := terraform.WithDefaultRetryableErrors(s.T(), &terraform.Options{
		TerraformDir: terraformDir,
		NoColor:      true,
	})

	pwd, err := terraform.OutputE(s.T(), terraformOptions, "admin_pass")
	logger.DebugCtx(s.ctx, err, "gathering pwd")
	s.Require().NoError(err)
	s.Require().NotEmpty(pwd)

	addr, err := terraform.OutputE(s.T(), terraformOptions, "address")
	logger.DebugCtx(s.ctx, err, "gathering addr",
		zap.String("addr", addr))
	s.Require().NoError(err)
	s.Require().NotEmpty(addr)

	const username = "Administrator"
	const timeout = 10 * time.Minute

	psrp := winrs.NewWinRS(username, pwd, addr, timeout)
	logger.DebugCtx(s.ctx, err, "creating winrs client",
		zap.String("username", username),
		zap.String("pwd", pwd),
		zap.String("addr", addr),
		zap.Duration("timeout", timeout))

	// noop w8 for connection
	err = psrp.Echo()
	logger.DebugCtx(s.ctx, err, "connecting via psrp")
	s.Require().NoError(err)

	s.psrp = psrp

	// upload updater 2 temp if needed

	hRemote, err := s.psrp.GetFileSha256(updaterAtTemp)
	logger.DebugCtx(s.ctx, err, "get remote file hash",
		zap.String("path", updaterAtTemp),
		zap.String("hash", hRemote))
	s.Require().NoError(err)

	hLocal, err := getFilehash(updaterFilename)
	logger.DebugCtx(s.ctx, err, "get local file hash",
		zap.String("path", updaterFilename),
		zap.String("hash", hLocal))
	s.Require().NoError(err)

	if hLocal != hRemote {
		s.Require().NoError(s.psrp.UploadFile(updaterAtTemp, updaterFilename))
	}
}

func getFilehash(path string) (hash string, err error) {
	f, err := os.Open(path)
	if err != nil {
		return
	}
	defer func() {
		fErr := f.Close()
		if err == nil {
			err = fErr
		}
	}()

	h := sha256.New()
	_, err = io.Copy(h, f)
	if err != nil {
		return
	}
	hash = fmt.Sprintf("%x", h.Sum(nil))

	return
}

const updaterDir = `C:\Program Files\Yandex.Cloud\Guest Agent Updater\`
const schedTaskName = `yc-guest-agent-updater`
const updaterLogpath = `C:\Windows\Temp\guest-agent-updater.log`

func (s *guestAgentUpdaterSuite) SetupTest() {
	logger.DebugCtx(s.ctx, nil, "******************** SETUP TEST ********************")

	// create `updaterDir`
	err := s.psrp.CreateDirAll(updaterDir)
	logger.DebugCtx(s.ctx, err, "create dir",
		zap.String("path", updaterDir))
	s.Require().NoError(err)

	// copy updater
	err = s.psrp.RemoteCopy(updaterDir, updaterAtTemp)
	logger.DebugCtx(s.ctx, err, "copy",
		zap.String("dst", updaterDir),
		zap.String("src", updaterAtTemp))
	s.Require().NoError(err)

	// create task scheduler job
	cmdFilepath := `C:\Windows\System32\cmd.exe`
	args := fmt.Sprintf(`/c "%v" update --log-level debug > %v`,
		updaterDir+updaterFilename,
		updaterLogpath)
	err = s.psrp.CreateTaskSchedulerJob(cmdFilepath, args, schedTaskName)
	logger.DebugCtx(s.ctx, err, "create task scheduler job",
		zap.String("path", cmdFilepath),
		zap.String("arg", args),
		zap.String("name", schedTaskName))
	s.Require().NoError(err)
}

func (s *guestAgentUpdaterSuite) TearDownTest() {
	// remove task scheduler job
	err := s.psrp.RemoveTaskSchedulerJob(schedTaskName)
	logger.DebugCtx(s.ctx, err, "create task scheduler job",
		zap.String("name", schedTaskName))
	s.Require().NoError(err)

	// remove service
	err = s.psrp.GuestAgentUpdaterRemove(updaterAtTemp)
	logger.DebugCtx(s.ctx, err, "call updater to remove yc-guest-agent service",
		zap.String("filepath", updaterAtTemp))
	s.Require().NoError(err)

	const ycDir = `C:\Program Files\Yandex.Cloud`

	// print log
	err = s.psrp.GetContent(updaterLogpath)
	logger.DebugCtx(s.ctx, err, "get logs",
		zap.String("path", updaterLogpath))
	s.Require().NoError(err)

	// remove `Yandex.Cloud dir`
	err = s.psrp.RemoveDir(ycDir)
	logger.DebugCtx(s.ctx, err, "remove dir",
		zap.String("path", ycDir))
	s.Require().NoError(err)
}

const guestAgentLatest = `https://storage.yandexcloud.net/yandexcloud-guestagent/release/stable`

func getLatestVersion() (string, error) {
	r, err := resty.New().R().Get(guestAgentLatest)
	if err != nil {
		return "", err
	}
	if r.IsError() {
		return "", err
	}

	return strings.Trim(string(r.Body()), "\n"), nil
}

const serviceName = "yc-guest-agent"

func (s *guestAgentUpdaterSuite) TestInstall() {
	// run scheduled task
	err := s.psrp.StartTaskSchedulerJob(schedTaskName)
	logger.DebugCtx(s.ctx, err, "start scheduled task",
		zap.String("name", schedTaskName))
	s.NoError(err)

	// get service status
	err = s.psrp.WaitServiceRunning(serviceName)
	logger.DebugCtx(s.ctx, err, "wait service status",
		zap.String("name", serviceName))
	s.NoError(err)
}

const repoDir = updaterDir + `Local Repository\`
const versionsTemplate = `https://storage.yandexcloud.net/yandexcloud-guestagent/release/%v/windows/amd64/%v`
const guestAgentFilename = `guest-agent.exe`
const guestAgentChecksum = guestAgentFilename + `.sha256`

func addGAVersionToRepository(p *winrs.WinRS, v string) error {
	if err := p.CreateDirAll(repoDir + v); err != nil {
		return err
	}

	a := fmt.Sprintf(versionsTemplate, v, guestAgentFilename)
	c := fmt.Sprintf(versionsTemplate, v, guestAgentChecksum)

	if err := p.Download(a, repoDir+v+`\`+guestAgentFilename); err != nil {
		return err
	}

	return p.Download(c, repoDir+v+`\`+guestAgentChecksum)
}

const guestAgentDir = `C:\Program Files\Yandex.Cloud\Guest Agent`
const guestAgentFilepath = guestAgentDir + `\` + guestAgentFilename

func installGAVersion(p *winrs.WinRS, v string) error {
	if err := p.CreateDirAll(guestAgentDir); err != nil {
		return err
	}

	a := fmt.Sprintf(versionsTemplate, v, guestAgentFilename)
	o := guestAgentFilepath

	if err := p.Download(a, o); err != nil {
		return err
	}

	return p.GuestAgentInstall(o)
}

func (s *guestAgentUpdaterSuite) TestUpdate() {
	// install non-latest agent
	nonLatestVersion := "0.0.5"
	err := installGAVersion(s.psrp, nonLatestVersion)
	logger.DebugCtx(s.ctx, err, "install non-latest guest agent version",
		zap.String("version", nonLatestVersion))
	s.NoError(err)

	// run scheduled task
	err = s.psrp.StartTaskSchedulerJob(schedTaskName)
	logger.DebugCtx(s.ctx, err, "start scheduled task",
		zap.String("name", schedTaskName))
	s.NoError(err)

	// get service status
	err = s.psrp.WaitServiceRunning(serviceName)
	logger.DebugCtx(s.ctx, err, "wait service status",
		zap.String("name", serviceName))
	s.NoError(err)

	// check if latest installed
	latestVersion, err := getLatestVersion()
	logger.DebugCtx(s.ctx, err, "get latest version",
		zap.String("version", latestVersion))
	s.NoError(err)

	err = s.psrp.WaitGuestAgentVersionToBecome(guestAgentFilepath, latestVersion)
	logger.DebugCtx(s.ctx, err, "check installed latest version")
	s.NoError(err)

	// check folders for both
	nonLatestFilepath := repoDir + nonLatestVersion + `\` + guestAgentFilename
	err = s.psrp.TestFile(nonLatestFilepath)
	logger.DebugCtx(s.ctx, err, "check file exists",
		zap.String("filepath", nonLatestFilepath))
	s.NoError(err)

	latestFilepath := repoDir + latestVersion + `\` + guestAgentFilename
	err = s.psrp.TestFile(latestFilepath)
	logger.DebugCtx(s.ctx, err, "check file exists",
		zap.String("filepath", latestFilepath))
	s.NoError(err)
}

func (s *guestAgentUpdaterSuite) TestUpdateAndCaptureVersion() {
	// install non-latest agent
	nonLatestVersion := "0.0.5"
	err := installGAVersion(s.psrp, nonLatestVersion)
	logger.DebugCtx(s.ctx, err, "install non-latest guest agent version",
		zap.String("version", nonLatestVersion))
	s.NoError(err)

	// run scheduled task
	err = s.psrp.StartTaskSchedulerJob(schedTaskName)
	logger.DebugCtx(s.ctx, err, "start scheduled task",
		zap.String("name", schedTaskName))
	s.NoError(err)

	// get service status
	err = s.psrp.WaitServiceRunning(serviceName)
	logger.DebugCtx(s.ctx, err, "wait service status",
		zap.String("name", serviceName))
	s.NoError(err)

	// check if latest installed
	latestVersion, err := getLatestVersion()
	logger.DebugCtx(s.ctx, err, "get latest version",
		zap.String("version", latestVersion))
	s.NoError(err)

	err = s.psrp.WaitGuestAgentVersionToBecome(guestAgentFilepath, latestVersion)
	logger.DebugCtx(s.ctx, err, "check installed latest version")
	s.NoError(err)

	// check folders for both
	nonLatestFilepath := repoDir + nonLatestVersion + `\` + guestAgentFilename
	err = s.psrp.TestFile(nonLatestFilepath)
	logger.DebugCtx(s.ctx, err, "check file exists",
		zap.String("filepath", nonLatestFilepath))
	s.NoError(err)

	latestFilepath := repoDir + latestVersion + `\` + guestAgentFilename
	err = s.psrp.TestFile(latestFilepath)
	logger.DebugCtx(s.ctx, err, "check file exists",
		zap.String("filepath", latestFilepath))
	s.NoError(err)
}

func (s *guestAgentUpdaterSuite) TestUpdateWithRunningAgent() {
	// install non-latest agent
	nonLatestVersion := "0.0.5"
	err := installGAVersion(s.psrp, nonLatestVersion)
	logger.DebugCtx(s.ctx, err, "install non-latest guest agent version",
		zap.String("version", nonLatestVersion))
	s.NoError(err)

	err = addGAVersionToRepository(s.psrp, nonLatestVersion)
	logger.DebugCtx(s.ctx, err, "add non-latest guest agent version to repository",
		zap.String("version", nonLatestVersion))
	s.NoError(err)

	// start non-latest agent
	err = s.psrp.StartService(serviceName)
	logger.DebugCtx(s.ctx, err, "wait service status",
		zap.String("name", serviceName))
	s.NoError(err)

	// wait service to start
	err = s.psrp.WaitServiceRunning(serviceName)
	logger.DebugCtx(s.ctx, err, "wait service status",
		zap.String("name", serviceName))
	s.NoError(err)

	// run scheduled task
	err = s.psrp.StartTaskSchedulerJob(schedTaskName)
	logger.DebugCtx(s.ctx, err, "start scheduled task",
		zap.String("name", schedTaskName))
	s.NoError(err)

	// wait service to start
	err = s.psrp.WaitServiceRunning(serviceName)
	logger.DebugCtx(s.ctx, err, "wait service status",
		zap.String("name", serviceName))
	s.NoError(err)

	// check if latest installed
	latestVersion, err := getLatestVersion()
	logger.DebugCtx(s.ctx, err, "get latest version",
		zap.String("version", latestVersion))
	s.NoError(err)

	err = s.psrp.WaitGuestAgentVersionToBecome(guestAgentFilepath, latestVersion)
	logger.DebugCtx(s.ctx, err, "check installed latest version")
	s.NoError(err)

	// check folders for both
	nonLatestFilepath := repoDir + nonLatestVersion + `\` + guestAgentFilename
	err = s.psrp.TestFile(nonLatestFilepath)
	logger.DebugCtx(s.ctx, err, "check file exists",
		zap.String("filepath", nonLatestFilepath))
	s.NoError(err)

	latestFilepath := repoDir + latestVersion + `\` + guestAgentFilename
	err = s.psrp.TestFile(latestFilepath)
	logger.DebugCtx(s.ctx, err, "check file exists",
		zap.String("filepath", latestFilepath))
	s.NoError(err)
}

func (s *guestAgentUpdaterSuite) TestRepoRotation() {
	// install latest agent
	latestVersion, err := getLatestVersion()
	logger.DebugCtx(s.ctx, err, "get latest version",
		zap.String("version", latestVersion))
	s.NoError(err)

	err = installGAVersion(s.psrp, latestVersion)
	logger.DebugCtx(s.ctx, err, "install non-latest guest agent version",
		zap.String("version", latestVersion))
	s.NoError(err)

	// start non-latest agent
	err = s.psrp.StartService(serviceName)
	logger.DebugCtx(s.ctx, err, "start service",
		zap.String("service name", serviceName))
	s.NoError(err)

	// add 6 agents
	versionToEvict := "0.0.1"
	for _, v := range []string{versionToEvict, "0.0.2", "0.0.3", "0.0.4", "0.0.5", latestVersion} {
		err = addGAVersionToRepository(s.psrp, v)
		logger.DebugCtx(s.ctx, err, "add guest agent version into repository",
			zap.String("version", v))
		s.NoError(err)
	}

	// run scheduled task
	err = s.psrp.StartTaskSchedulerJob(schedTaskName)
	logger.DebugCtx(s.ctx, err, "start scheduled task",
		zap.String("name", schedTaskName))
	s.NoError(err)

	// wait service to start
	err = s.psrp.WaitServiceRunning(serviceName)
	logger.DebugCtx(s.ctx, err, "wait service status",
		zap.String("name", serviceName))
	s.NoError(err)

	// check folders
	for _, v := range []string{"0.0.2", "0.0.3", "0.0.4", "0.0.5", latestVersion} {
		gaFilepath := repoDir + v + `\` + guestAgentFilename
		err = s.psrp.TestFile(gaFilepath)
		logger.DebugCtx(s.ctx, err, "check file exists",
			zap.String("filepath", gaFilepath))
		s.NoError(err)

		csFilepath := repoDir + v + `\` + guestAgentChecksum
		err = s.psrp.TestFile(csFilepath)
		s.NoError(err)
	}

	evictedVersionFilepath := repoDir + versionToEvict + `\` + guestAgentFilename
	err = s.psrp.TestFile(evictedVersionFilepath)
	logger.DebugCtx(s.ctx, err, "check file not exists",
		zap.String("filepath", evictedVersionFilepath))
	s.Error(err)
}
