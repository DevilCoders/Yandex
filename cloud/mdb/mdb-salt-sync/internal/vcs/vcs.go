package vcs

import (
	"bytes"
	"context"
	"encoding/xml"
	"fmt"
	"os"
	"os/exec"
	"path"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/mdb-salt-sync/internal/repos"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type CheckoutOptions struct {
	Version    string
	UpdatePath string
	Exclude    []string
}

var pullRetry = retry.New(retry.Config{
	MaxRetries: 5,
})

func CheckoutVersion(o string) CheckoutOption {
	return func(args *CheckoutOptions) {
		args.Version = o
	}
}

func CheckoutUpdatePath(o string) CheckoutOption {
	return func(args *CheckoutOptions) {
		args.UpdatePath = o
	}
}

func CheckoutExclude(o []string) CheckoutOption {
	return func(args *CheckoutOptions) {
		args.Exclude = o
	}
}

type CheckoutOption func(*CheckoutOptions)

type Head struct {
	Version string
	Time    time.Time
}

type Provider interface {
	fmt.Stringer

	LatestVersionSymbol() string
	Checkout(opts ...CheckoutOption) error
	Pull() error
	LocalHead() (Head, error)
	RemoteHead() (Head, error)
	GetMountPath() string
	RemoveEntireVCSMount() error
}

func getCheckoutOptions(vcs Provider, opts []CheckoutOption) *CheckoutOptions {
	r := &CheckoutOptions{
		Version: vcs.LatestVersionSymbol(),
	}
	for _, optSetter := range opts {
		optSetter(r)
	}
	return r
}

type Factory struct {
	CheckoutsPath string
}

func (f *Factory) NewProvider(cfg repos.RepoConfig, mountPrefix string, l log.Logger) Provider {
	if strings.HasPrefix(cfg.URI, "git@") || strings.HasSuffix(cfg.URI, ".git") {
		return &GitProvider{
			RepoURI:       cfg.URI,
			MountPath:     path.Join(mountPrefix, cfg.Mount),
			L:             log.With(l, log.String("vcs", "git")),
			CheckoutsPath: f.CheckoutsPath,
		}
	}

	return &SVNProvider{
		RepoURI:       cfg.URI,
		RepoPath:      cfg.Path,
		MountPath:     path.Join(mountPrefix, cfg.Mount),
		L:             log.With(l, log.String("vcs", "svn")),
		CheckoutsPath: f.CheckoutsPath,
	}
}

func checkMountPathSafeDelete(p string) {
	if p == "" || p == "/" {
		panic(fmt.Sprintf("path is unsafe for delete: %s", p))
	}
}

func removeEntirePath(p string, l log.Logger) error {
	checkMountPathSafeDelete(p)
	cmd := exec.Command("rm", "-rf", p)
	_, err := Execute(cmd, l)
	return err
}

type SVNProvider struct {
	RepoURI       string
	RepoPath      string
	MountPath     string
	CheckoutsPath string
	L             log.Logger
}

var _ Provider = &SVNProvider{}

func (p *SVNProvider) String() string {
	return fmt.Sprintf("RepoURI: %s, RepoPath: %s, MountPath: %s", p.RepoURI, p.RepoPath, p.MountPath)
}

func (p *SVNProvider) GetMountPath() string {
	return p.MountPath
}

func (p *SVNProvider) LatestVersionSymbol() string {
	return "HEAD"
}

func (p *SVNProvider) RemoveEntireVCSMount() error {
	return removeEntirePath(p.MountPath, p.L)
}

func (p *SVNProvider) Execute(args ...string) (ExecuteResult, error) {
	cmd := exec.Command("svn", args...)
	return Execute(cmd, p.L)
}

func (p *SVNProvider) Checkout(checkoutOpts ...CheckoutOption) error {
	opts := getCheckoutOptions(p, checkoutOpts)
	if p.CheckoutsPath == "" {
		if opts.UpdatePath != "" {
			return xerrors.Errorf("partial checkouts into mount not implemented in SVNProvider")
		}
		return p.checkoutInto(p.MountPath, opts.Version)
	}

	cachePath := pathToCheckout(p.CheckoutsPath, p.RepoURI+p.RepoPath, opts.Version)
	ok, err := dirExists(cachePath)
	if err != nil {
		return err
	}
	if !ok {
		if err := p.checkoutInto(cachePath, opts.Version); err != nil {
			return err
		}
	}

	syncFrom := path.Join(cachePath, opts.UpdatePath) + "/"
	mountPath := path.Join(p.MountPath, opts.UpdatePath)
	return rsync(syncFrom, mountPath, opts.Exclude, p.L)
}

func (p *SVNProvider) checkoutInto(checkoutPath, version string) error {
	p.L.Debug("Executing svn checkout", log.String("repo uri", p.RepoURI), log.String("repo path", p.RepoPath))
	if _, err := p.Execute("checkout", p.RepoURI+p.RepoPath, checkoutPath, "-r", version); err != nil {
		return err
	}
	return nil
}

func (p *SVNProvider) Pull() error {
	if p.CheckoutsPath == "" {
		return xerrors.Errorf("pull into mount not implemented in SVNProvider")
	}
	cachePath := pathToCheckout(p.CheckoutsPath, p.RepoURI+p.RepoPath, p.LatestVersionSymbol())
	ok, err := dirExists(cachePath)

	if err != nil {
		return err
	}
	if !ok {
		return p.checkoutInto(cachePath, p.LatestVersionSymbol())
	}
	p.L.Debug("Executing svn update")
	// There are a lot of connection problems - Retry them.
	return pullRetry.RetryWithLog(
		context.Background(),
		func() error {
			_, err = p.Execute("update", cachePath, "-r", p.LatestVersionSymbol())
			if err != nil {
				if safeErrorForSVNUpdate(err) {
					return err
				}
				return retry.Permanent(err)
			}
			return nil
		},
		"svn update",
		p.L,
	)
}

func safeErrorForSVNUpdate(err error) bool {
	// svn: E210002: To better debug SSH connection problems ...
	//		svn: E210002: Network connection closed unexpectedly
	var execErr *ExecutionFailed
	if xerrors.As(err, &execErr) {
		if strings.Contains(execErr.Stderr, "svn: E210002: Network connection closed unexpectedly") {
			return true
		}
	}
	return false
}

func (p *SVNProvider) LocalHead() (Head, error) {
	infoPath := pathToCheckout(p.CheckoutsPath, p.RepoURI+p.RepoPath, p.LatestVersionSymbol())
	if p.CheckoutsPath == "" {
		infoPath = p.MountPath
	}
	p.L.Debug("Executing local svn info", log.String("path", infoPath))
	return p.head(infoPath)
}

func (p *SVNProvider) RemoteHead() (Head, error) {
	p.L.Debug("Executing remote svn info")
	return p.head(p.RepoURI + p.RepoPath)
}

func (p *SVNProvider) head(arg string) (Head, error) {
	res, err := p.Execute("info", arg, "--xml")
	if err != nil {
		return Head{}, err
	}

	return lastChangedFromSVNInfo(res.Stdout.String())
}

func lastChangedFromSVNInfo(infoXML string) (Head, error) {
	var info struct {
		Entry struct {
			Commit struct {
				Revision string `xml:"revision,attr"`
				Date     string `xml:"date"`
			} `xml:"commit"`
		} `xml:"entry"`
	}

	if err := xml.Unmarshal([]byte(infoXML), &info); err != nil {
		return Head{}, xerrors.Errorf("failed to parse svn info: %w", err)
	}

	if info.Entry.Commit.Revision == "" {
		return Head{}, xerrors.New("empty revision")
	}
	t, err := time.Parse(time.RFC3339, info.Entry.Commit.Date)
	if err != nil {
		return Head{}, xerrors.Errorf("failed to parse date in svn info: %w", err)
	}

	return Head{Version: info.Entry.Commit.Revision, Time: t}, nil
}

type GitProvider struct {
	RepoURI       string
	MountPath     string
	CheckoutsPath string
	L             log.Logger
}

var _ Provider = &GitProvider{}

func (p *GitProvider) String() string {
	return fmt.Sprintf("RepoURI: %s, MountPath: %s", p.RepoURI, p.MountPath)
}

func (p *GitProvider) GetMountPath() string {
	return p.MountPath
}

func (p *GitProvider) DeleteRepoDir() error {
	checkMountPathSafeDelete(p.MountPath)
	cmd := exec.Command("rm", "-rf", path.Join(p.MountPath, ".git"))
	_, err := Execute(cmd, p.L)
	return err
}

func (p *GitProvider) RemoveEntireVCSMount() error {
	return removeEntirePath(p.MountPath, p.L)
}

func (p *GitProvider) Execute(args ...string) (ExecuteResult, error) {
	cmd := exec.Command("git", args...)
	return Execute(cmd, p.L)
}

func (p *GitProvider) LatestVersionSymbol() string {
	return "HEAD"
}

func (p *GitProvider) Checkout(checkoutOpts ...CheckoutOption) error {
	opts := getCheckoutOptions(p, checkoutOpts)
	if p.CheckoutsPath == "" {
		if opts.UpdatePath != "" {
			return xerrors.Errorf("partial checkouts into mount not implement in GitProvider")
		}
		return p.clone(p.MountPath)
	}
	cachePath := pathToCheckout(p.CheckoutsPath, p.RepoURI, opts.Version)
	ok, err := dirExists(cachePath)
	if err != nil {
		return err
	}
	if !ok {
		if err := p.clone(cachePath); err != nil {
			return err
		}
		if opts.Version != p.LatestVersionSymbol() {
			p.L.Debug("Executing git checkout", log.String("repo uri", p.RepoURI))
			_, err = p.Execute("-C", cachePath, "checkout", opts.Version)
			if err != nil {
				return err
			}
		}
	}

	syncFrom := path.Join(cachePath, opts.UpdatePath) + "/"
	mountPath := path.Join(p.MountPath, opts.UpdatePath)
	return rsync(syncFrom, mountPath, opts.Exclude, p.L)
}

func (p *GitProvider) clone(clonePath string) error {
	p.L.Debug("Executing git clone", log.String("repo uri", p.RepoURI), log.String("path", clonePath))
	_, err := p.Execute("clone", p.RepoURI, clonePath)
	return err
}

func (p *GitProvider) Pull() error {
	if p.CheckoutsPath == "" {
		return xerrors.Errorf("pull into mount not implemented in GitProvider")
	}
	cachePath := pathToCheckout(p.CheckoutsPath, p.RepoURI, p.LatestVersionSymbol())
	ok, err := dirExists(cachePath)
	if err != nil {
		return err
	}
	if !ok {
		return p.clone(cachePath)
	}
	p.L.Debug("Executing git pull")
	_, err = p.Execute("-C", cachePath, "pull")
	return err
}

func (p *GitProvider) LocalHead() (Head, error) {
	infoPath := pathToCheckout(p.CheckoutsPath, p.RepoURI, p.LatestVersionSymbol())
	if p.CheckoutsPath == "" {
		infoPath = p.MountPath
	}
	p.L.Debug("Executing local git rev-pa	rse", log.String("path", infoPath))
	res, err := p.Execute("-C", infoPath, "rev-parse", "HEAD")
	return Head{Version: strings.TrimSpace(res.Stdout.String())}, err
}

func (p *GitProvider) RemoteHead() (Head, error) {
	return Head{}, xerrors.New("remote head not implemented for git provider")
}

func rsync(fromPath, toPath string, exclude []string, l log.Logger) error {
	if _, err := Execute(exec.Command("mkdir", "-p", toPath), l); err != nil {
		return err
	}
	rsyncArgs := []string{
		"-ahvW", "--delete", "--inplace", fromPath, toPath, "--exclude", ".svn/", "--exclude", ".git/",
	}
	for _, ex := range exclude {
		rsyncArgs = append(rsyncArgs, "--exclude", "/"+ex+"/")
	}
	cmd := exec.Command("rsync", rsyncArgs...)
	_, err := Execute(cmd, l)
	return err
}

type ExecuteResult struct {
	Stdout bytes.Buffer
	Stderr bytes.Buffer
}

type ExecutionFailed struct {
	Cmd        *exec.Cmd
	Stderr     string
	InnerError error
}

func (e *ExecutionFailed) Error() string {
	return fmt.Sprintf("command %s (%s) failed with: %s", e.Cmd, e.Stderr, e.InnerError)
}

func Execute(cmd *exec.Cmd, l log.Logger) (ExecuteResult, error) {
	l.Debug("Executing command", log.String("path", cmd.Path), log.Strings("args", cmd.Args))
	var res ExecuteResult
	cmd.Stdout = &res.Stdout
	cmd.Stderr = &res.Stderr
	startAt := time.Now()

	if err := cmd.Run(); err != nil {
		logCommandResult(l, "Command failed to execute.", res)
		return res, &ExecutionFailed{Cmd: cmd, Stderr: res.Stderr.String(), InnerError: err}
	}

	logCommandResult(l, fmt.Sprintf("Command executed (within %s).", time.Since(startAt)), res)
	return res, nil
}

func logCommandResult(L log.Logger, logstr string, res ExecuteResult) {
	str := res.Stdout.String()
	if len(strings.Trim(str, "\n")) > 0 {
		str = logstr + "\nStdout:\n" + str
	}
	L.Debug(str)

	str = res.Stderr.String()
	if len(strings.Trim(str, "\n")) > 0 {
		str = logstr + "\nStderr:\n" + str
	}
	L.Debug(str)
}

func dirExists(p string) (bool, error) {
	fi, err := os.Stat(p)
	if err != nil {
		if os.IsNotExist(err) {
			return false, nil
		}
		return false, xerrors.Errorf("failed to stat %q: %w", p, err)
	}
	if !fi.Mode().IsDir() {
		return false, xerrors.Errorf("path %q exists, but not a directory: %s", p, fi.Mode())
	}
	return true, nil
}
