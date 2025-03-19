package sftpd

import "flag"

// Flags are flags for sftpd
var Flags = struct {
	AuthorizedKeys string
	PrivateKey     string
	ListenAddr     string
	LogLevel       string
}{}

func init() {
	//ConnectionTimeout int // TODO
	//IdleTimeout       int // TODO

	flag.StringVar(&Flags.AuthorizedKeys, "authorized-keys", "sftpd-authorized_keys", "Path to the authorized_keys file")
	flag.StringVar(&Flags.PrivateKey, "private-key", "sftpd-id_rsa", "Path to the server private key file")
	flag.StringVar(&Flags.ListenAddr, "listen-addr", "[::]:2022", "Address to bind sftpd server to")
	flag.StringVar(&Flags.LogLevel, "log-level", "error", "Enable logging (error|warn|info|debug)")
}
