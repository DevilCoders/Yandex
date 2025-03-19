package sender

import (
	"net"
	"time"

	"github.com/prometheus/client_golang/prometheus"
)

var dialFunc = net.DialTimeout

// createGraphiteConn create a conn to graphite server
func (s *Sender) createGraphiteConn(srv string, connTime time.Duration) net.Conn {
	var (
		conn net.Conn
		err  error
	)

	maxReconnect := s.config.Graphite.MaxReconnect
	minConnTime := time.Duration(s.config.Graphite.MinConnTimeMs) * time.Millisecond
	maxConnTime := time.Duration(s.config.Graphite.MaxConnTimeMs) * time.Millisecond

	// maxReconnect + 1 where
	// 1 is normal connect
	// maxReconnect is reconnection attempts
	dialTime := time.Duration(int(connTime)/maxReconnect + 1)
	if dialTime < minConnTime {
		originMaxReconnect := maxReconnect
		maxReconnect = int(connTime/minConnTime) - 1
		if maxReconnect <= 0 {
			maxReconnect = 1
		}
		logger.Warnf("Reconnect interval too small, reduce maxReconnect %d -> %d, dialTime=%s",
			originMaxReconnect, maxReconnect, dialTime)
		dialTime = time.Duration(int(connTime) / maxReconnect)
	} else if dialTime > maxConnTime {
		dialTime = maxConnTime
	}
	for i := 0; i <= maxReconnect && conn == nil; i++ {
		if i > 0 {
			logger.Infof("Reconnect attempt=%d, host=%s", i, srv)
		}
		connStart := time.Now()
		conn, err = dialFunc("tcp", srv, dialTime)
		metricsDuration("connect."+srv, connStart)
		if err != nil {
			if conn != nil {
				_ = conn.Close()
				conn = nil
			}
			if time.Since(connStart) < dialTime {
				sleepTime := minDuration(dialTime/3, dialTime-time.Since(connStart))
				logger.Debugf("Slowing down reconnection attempts by %s", sleepTime)
				time.Sleep(sleepTime)
			}
		}
	}
	if err != nil {
		logger.Warnf("Can not connect to host=%s: %v", srv, err)
		return nil
	}
	_ = conn.SetDeadline(time.Time{})
	return conn
}

// sendToGraphite takes metrics as []byte and sends to to graphite servers
// It returns error if it failed to send data
func sendToGraphite(addr string, conn net.Conn, buf *senderBuffer) error {
	defer metricsDuration("send."+addr, time.Now())
	defer func() { _ = conn.Close() }()

	logger.Debugf("Sent metric host=%s", addr)
	// TODO(skacheev) handle partial sent?
	_, err := conn.Write(buf.bytes())
	if err != nil {
		return err
	}
	metricsSendToServer.With(prometheus.Labels{"server": addr}).Add(float64(buf.len()))
	return nil
}

func minDuration(a time.Duration, b time.Duration) time.Duration {
	if a > b {
		return b
	}
	return a
}
