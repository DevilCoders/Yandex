package sftpd

import (
	"encoding/json"
	"io"
	"io/ioutil"
	"net"
	"os"

	"golang.org/x/crypto/ssh"

	"a.yandex-team.ru/admins/ftp2s3/internal/logger"
	s3driver "a.yandex-team.ru/admins/ftp2s3/internal/s3-driver"
	"a.yandex-team.ru/admins/ftp2s3/internal/stats"
	liblog "a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var log liblog.Logger

var (
	ErrUnknownUser      = xerrors.New("sftpd/server: unknown user")
	ErrInvalidPublicKey = xerrors.New("sftpd/server: invalid public key")
)

type sftpdServer struct {
	config    *ssh.ServerConfig
	S3Service s3driver.S3API
	metrics   metrics.Registry
}

func loadAuthorizedKeys(in io.Reader) (map[string]string, error) {
	authorizedKeysBytes, err := ioutil.ReadAll(in)
	if err != nil {
		return nil, err
	}

	users := map[string]string{}
	err = json.Unmarshal(authorizedKeysBytes, &users)
	if err != nil {
		return nil, xerrors.Errorf("Load authorizedKeys db: %v", err)
	}

	keys := map[string]string{}
	for user, pubKeyString := range users {
		pubKey, _, _, _, err := ssh.ParseAuthorizedKey([]byte(pubKeyString))
		if err != nil {
			return nil, xerrors.Errorf("Malformed pubKey %q: %v: %w", user, err, ErrInvalidPublicKey)
		}
		keys[user] = string(pubKey.Marshal())
	}
	return keys, nil
}

func createServerConfig(authKeys map[string]string, r metrics.Registry) *ssh.ServerConfig {

	stats := stats.GetAuthStats(r)

	return &ssh.ServerConfig{
		PublicKeyCallback: func(c ssh.ConnMetadata, key ssh.PublicKey) (*ssh.Permissions, error) {
			fingerprint := ssh.FingerprintSHA256(key)
			user := c.User()

			var err error
			if pubKey, ok := authKeys[user]; ok {
				if pubKey == string(key.Marshal()) {
					stats.Success.Inc()
					return &ssh.Permissions{
						Extensions: map[string]string{"pubkey-id": fingerprint},
					}, nil
				}
				err = xerrors.Errorf("User %q: %w", user, ErrInvalidPublicKey)
			} else {
				stats.Unknown.Inc()
				err = xerrors.Errorf("User %q: %w", user, ErrUnknownUser)
			}
			stats.Failed.Inc()
			log.Errorf("%v", err)
			return nil, err
		},
	}
}

func configureSFTPServer(m *solomon.Registry) *ssh.ServerConfig {
	authKeysFD, err := os.Open(Flags.AuthorizedKeys)
	if err != nil {
		log.Fatalf("open authorized_keys file %s: %v", Flags.AuthorizedKeys, err)
	}
	defer authKeysFD.Close()
	authKeys, err := loadAuthorizedKeys(authKeysFD)
	if err != nil {
		log.Fatalf("loadAuthorizedKeys: %v", err)
	}

	privBytes, err := ioutil.ReadFile(Flags.PrivateKey)
	if err != nil {
		log.Fatalf("read private key %s: %v", Flags.PrivateKey, err)
	}
	private, err := ssh.ParsePrivateKey(privBytes)
	if err != nil {
		log.Fatalf("failed to load private key: %v", err)
	}

	config := createServerConfig(authKeys, m)
	config.AddHostKey(private)
	return config
}

// Serve sftp clients
func Serve() {
	log = logger.Set(Flags.LogLevel)
	log.Debugf("Flags %s", Flags)

	metrics := stats.NewStatsRegistry(log)
	go stats.ServeStats(metrics)

	s3api := s3driver.NewS3Service(log, logger.IsDebug())

	server := &sftpdServer{
		config:    configureSFTPServer(metrics),
		S3Service: s3api,
		metrics:   metrics,
	}

	listener, err := net.Listen("tcp", Flags.ListenAddr)
	if err != nil {
		log.Fatalf("failed to listen for connections: %v", err)
	}

	for {
		conn, _ := listener.Accept()
		if conn != nil {
			go server.AcceptInboundConnection(conn)
		}
	}
}
