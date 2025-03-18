package oauth

import (
	"io/ioutil"
	"net"
	"os"
	"path/filepath"
	"strings"

	"github.com/mitchellh/go-homedir"
	"golang.org/x/crypto/ssh"
	"golang.org/x/crypto/ssh/agent"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type (
	SSHKeyring interface {
		Next() bool
		Signer() ssh.Signer
		Close() error
	}

	// SSHFileKeyring implements keyring, that holds private key from file.
	SSHFileKeyring struct {
		eof    bool
		signer ssh.Signer
	}

	// SSHByteKeyring implements keyring, that holds private key from byte slice.
	SSHByteKeyring struct {
		eof    bool
		signer ssh.Signer
	}

	// SSHAgentKeyring implements keyring, that use ssh-agent.
	SSHAgentKeyring struct {
		pos     int
		max     int
		conn    net.Conn
		signers []ssh.Signer
	}

	// SSHContainerKeyring implements keyring, that holds another keyring.
	// E.g. allows you to use multiple keyring at once.
	SSHContainerKeyring struct {
		pos      int
		max      int
		keyrings []SSHKeyring
	}
)

// AutodetectSSHKeyring returns list of auto-detected keyring.
func AutodetectSSHKeyring() ([]SSHKeyring, error) {
	var keyrings []SSHKeyring
	if keyring, err := NewSSHAgentKeyring(); err == nil {
		keyrings = append(keyrings, keyring)
	}

	if sshPath, err := homedir.Expand("~/.ssh"); err == nil {
		// OAuth support only rsa sign so far
		if keyring, err := NewSSHFileKeyring(filepath.Join(sshPath, "id_rsa")); err == nil {
			keyrings = append(keyrings, keyring)
		}
	}

	if len(keyrings) == 0 {
		return nil, xerrors.Errorf("failed to auto-detect any ssh keyrings")
	}

	return keyrings, nil
}

// NewSSHContainerKeyring returns a new SSHContainerKeyring for the given keyrings.
// You must .Close() it after use (all underlying keyrings will be closed).
func NewSSHContainerKeyring(keyring ...SSHKeyring) *SSHContainerKeyring {
	return &SSHContainerKeyring{
		pos:      0,
		keyrings: keyring,
		max:      len(keyring),
	}
}

func (k *SSHContainerKeyring) Next() bool {
	for ; k.pos < k.max; k.pos++ {
		if k.keyrings[k.pos].Next() {
			return true
		}
	}

	return false
}

func (k *SSHContainerKeyring) Signer() ssh.Signer {
	return k.keyrings[k.pos].Signer()
}

func (k *SSHContainerKeyring) Close() error {
	var errs []string
	for i := 0; i < len(k.keyrings); i++ {
		if err := k.keyrings[i].Close(); err != nil {
			errs = append(errs, err.Error())
		}
	}

	if len(errs) > 0 {
		return xerrors.Errorf("failed to close keyrings: %s", strings.Join(errs, "; "))
	}

	return nil
}

// NewSSHFileKeyring returns a new SSHFileKeyring. keyPath must be exists.
// You must .Close() it after use.
func NewSSHFileKeyring(keyPath string) (*SSHFileKeyring, error) {
	fp, err := os.Open(keyPath)
	if err != nil {
		return nil, xerrors.Errorf("failed to read private key: %w", err)
	}

	defer func() { _ = fp.Close() }()

	buf, err := ioutil.ReadAll(fp)
	if err != nil {
		return nil, xerrors.Errorf("failed to read private key %s: %w", keyPath, err)
	}

	signer, err := ssh.ParsePrivateKey(buf)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse private key %s: %w", keyPath, err)
	}

	return &SSHFileKeyring{
		signer: signer,
		eof:    false,
	}, nil
}

func (k *SSHFileKeyring) Next() bool {
	ok := !k.eof
	k.eof = true
	return ok
}

func (k *SSHFileKeyring) Signer() ssh.Signer {
	return k.signer
}

func (k *SSHFileKeyring) Close() error {
	return nil
}

// NewSSHByteKeyring returns a new SSHByteKeyring for the given keyrings.
// You must .Close() it after use.
func NewSSHByteKeyring(key []byte) (*SSHByteKeyring, error) {
	signer, err := ssh.ParsePrivateKey(key)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse private key: %w", err)
	}

	return &SSHByteKeyring{
		signer: signer,
		eof:    false,
	}, nil
}

func (k *SSHByteKeyring) Next() bool {
	ok := !k.eof
	k.eof = true
	return ok
}

func (k *SSHByteKeyring) Signer() ssh.Signer {
	return k.signer
}

func (k *SSHByteKeyring) Close() error {
	return nil
}

// NewSSHAgentKeyring returns a new SSHAgentKeyring.
// keyFingerprints parameter allows you to filter unwanted keys.
// For example, if you want to use only one ssh key - just put it sha256 fingerprint.
//
// You must .Close() it after use.
func NewSSHAgentKeyring(keyFingerprints ...string) (*SSHAgentKeyring, error) {
	agentSocket := os.Getenv("SSH_AUTH_SOCK")
	if agentSocket == "" {
		return nil, xerrors.New("Can't find ssh-agent socket, env[SSH_AUTH_SOCK] are empty")
	}

	conn, err := net.Dial("unix", agentSocket)
	if err != nil {
		return nil, xerrors.Errorf("failed to communicate with ssh-agent: %w", err)
	}

	agentClient := agent.NewClient(conn)
	agentSigners, err := agentClient.Signers()
	if err != nil {
		return nil, xerrors.Errorf("failed to list ssh-agent keys: %w", err)
	}

	var signers []ssh.Signer
	if len(keyFingerprints) == 0 {
		signers = agentSigners
	} else {
		for _, signer := range agentSigners {
			fp := ssh.FingerprintSHA256(signer.PublicKey())
			for _, expected := range keyFingerprints {
				if fp == expected {
					signers = append(signers, signer)
				}
			}
		}
	}

	if len(signers) == 0 {
		return nil, xerrors.Errorf("no suitable ssh-agent keys found")
	}

	return &SSHAgentKeyring{
		pos:     -1,
		conn:    conn,
		signers: signers,
		max:     len(signers) - 1,
	}, nil
}

func (k *SSHAgentKeyring) Next() bool {
	if k.pos < k.max {
		k.pos++
		return true
	}

	return false
}

func (k *SSHAgentKeyring) Signer() ssh.Signer {
	return k.signers[k.pos]
}

func (k *SSHAgentKeyring) Close() error {
	return k.conn.Close()
}
