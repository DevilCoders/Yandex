package sftpd

import (
	"io"
	"net"

	"github.com/pkg/sftp"
	"golang.org/x/crypto/ssh"
)

func (s *sftpdServer) AcceptInboundConnection(c net.Conn) {
	conn, chans, reqs, err := ssh.NewServerConn(c, s.config)
	if err != nil {
		log.Infof("failed to handshake: %v", err)
		return
	}
	log.Infof("user %q logged in with pubkey-id %s", conn.User(), conn.Permissions.Extensions["pubkey-id"])

	go ssh.DiscardRequests(reqs)

	for newChannel := range chans {

		if newChannel.ChannelType() != "session" {
			_ = newChannel.Reject(ssh.UnknownChannelType, "unknown channel type")
			continue
		}

		channel, requests, err := newChannel.Accept()
		if err != nil {
			log.Infof("could not accept on channel: %v", err)
			continue
		}

		go func(in <-chan *ssh.Request, c ssh.Channel) {
			for req := range in {
				ok := false

				switch req.Type {
				case "subsystem":
					if string(req.Payload[4:]) == "sftp" {
						ok = true
						go s.handleChannel(c, conn)
					}

				}
				err := req.Reply(ok, nil)
				if err != nil {
					log.Infof("sftp req.Reply error: %v", err)
				}
			}
		}(requests, channel)
	}
}

func (s *sftpdServer) handleChannel(channel ssh.Channel, m ssh.ConnMetadata) {
	root := newS3Handler(s.S3Service, m.User(), s.metrics)
	server := sftp.NewRequestServer(channel, root)
	if err := server.Serve(); err == io.EOF {
		_ = server.Close()
		log.Infof("sftp client %q exited session.", m.User())
	} else if err != nil {
		log.Infof("sftp server for %q completed with error: %v", m.User(), err)
	}
}
