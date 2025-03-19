package ftpd

import (
	"crypto/tls"
	"os"

	"github.com/fclairamb/ftpserver/server"

	logger "a.yandex-team.ru/admins/ftp2s3/internal/logger"
	s3driver "a.yandex-team.ru/admins/ftp2s3/internal/s3-driver"
	"a.yandex-team.ru/admins/ftp2s3/internal/stats"
	liblog "a.yandex-team.ru/library/go/core/log"
)

var (
	log   liblog.Logger
	debug = false
)

func (service *FtpdService) GetSettings() (*server.Settings, error) {
	return &server.Settings{
		ListenAddr:               Flags.Host + ":" + Flags.Port,
		PublicHost:               Flags.PublicIP,
		PassiveTransferPortRange: Flags.PassivePortRange.Range,
		ConnectionTimeout:        Flags.ConnectionTimeout,
		IdleTimeout:              Flags.IdleTimeout,
		ActiveTransferPortNon20:  true,
	}, nil
}

func (service *FtpdService) GetTLSConfig() (*tls.Config, error) {
	if Flags.TLSCert != "" && Flags.TLSKey != "" {
		cert, err := tls.LoadX509KeyPair(Flags.TLSCert, Flags.TLSKey)
		if err != nil {
			return nil, err
		}
		return &tls.Config{Certificates: []tls.Certificate{cert}}, nil
	}
	return nil, nil
}

// Serve ftp clients
func Serve() {
	log = logger.Set(Flags.LogLevel)
	log.Debugf("Flags %s", Flags)
	debug = logger.IsDebug()

	metrics := stats.NewStatsRegistry(log)
	go stats.ServeStats(metrics)

	service := &FtpdService{
		S3Service: s3driver.NewS3Service(log, debug),
		metrics:   metrics,
	}
	log.Infof("Loading passwd db from %s", Flags.Passwd)
	jsonFile, err := os.Open(Flags.Passwd)
	if err != nil {
		log.Fatalf("Failed to read userdb: %v", err)
	}
	if err := service.getUsers(jsonFile); err != nil {
		log.Fatalf("Error obtaining users DB: %v", err)
	}
	_ = jsonFile.Close()

	server := server.NewFtpServer(service)
	server.Logger = logger.NewFtpdLogger()

	if err := server.ListenAndServe(); err != nil {
		log.Fatalf("Error starting server: %v", err)
	}
}
