package sender

import (
	"bufio"
	"fmt"
	"io"
	"net"
	"os"
	"os/signal"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"
	"sync"
	"syscall"
	"time"

	dto "github.com/prometheus/client_model/go"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/log/zap/logrotate"
)

var logger log.Logger
var (
	err         error
	metricRegex = regexp.MustCompile(`^` + // bound at begin
		// name of metric must contains at last 3 parts
		// first, dot and second part, and last dot
		`(?:[-a-zA-Z0-9_]+\.){2}` +

		// last part is name
		// allow any non wite space symbols (XXX: may be not all non blank?)
		`\S+` +

		// delimiter between name and metric value
		`\s+` +

		// value is required validated part
		// we allo NaN, +/- Inf , floats (with .)
		// and exponent in the numbers (-/+[eE])
		`(?:NaN|[-+]?(?:Inf|[-+0-9.eE]+))` +

		// optional timestamp with leading delimiter
		// we capture timestamp for checking it presence
		`(?:\s+(\d{8,10}))?\n$`,
	)
	receiveMetricCh = make(chan []byte)
)

func init() {
	if metricRegex.NumSubexp() != 1 {
		logger.Fatal("ValidateMetric expect exactly one Subexpression!!!")
	}
}

// Sender receive metrics into buffer and pass it to send func
type Sender struct {
	store  *bufStore
	config *Config
}

// NewSender create sender instance
func NewSender(cfg *Config) *Sender {
	return &Sender{
		store:  newBufStore(cfg.Sender.DelayFactor),
		config: cfg,
	}
}

func (s *Sender) run() {
	sendInterval := s.config.Sender.SendInterval
	maxMetrics := s.config.Sender.MaxMetrics
	maxDelayedMetrics := maxMetrics * s.config.Sender.DelayFactor

	logger.Infof("Settings max_metrics=%d, max_delayed_metrics=%d, send_interval=%s",
		maxMetrics, maxDelayedMetrics, sendInterval)

	sendTicker := time.NewTicker(sendInterval)
	defer sendTicker.Stop()

	var ( // logging variables
		invalid float64
		receive int
		delay   int
		drop    int
	)
	delayedBuffer := [][]byte{}
	buf := s.store.getBuffer()

	// preserve buffered data
	sigc := make(chan os.Signal, 1)
	signal.Notify(sigc, syscall.SIGINT, syscall.SIGTERM, syscall.SIGQUIT)
	go func() {
		sig := <-sigc
		logger.Infof("Receive signal=%s", sig.String())
		s.saveBuffersAtExit(buf, delayedBuffer)
		os.Exit(0)
	}()

	for {
		select {
		case metric := <-receiveMetricCh:
			receive++
			metricsReceive.Inc()
			metricsLastReceive.SetToCurrentTime()
			if buf.len() > maxMetrics {
				// TODO (skacheev) use ring buffer here
				// we want drop oldest metrics, not newest
				if len(delayedBuffer) < maxDelayedMetrics {
					delay++
					metricsDelay.Inc()
					delayedBuffer = append(delayedBuffer, metric)
					break
				}
				drop++
				metricsDrop.Inc()
				break
			}
			if logTrace {
				logger.Debugf("Add to buf metric=%q", string(metric))
			}
			buf.add(metric)
		case <-sendTicker.C:
			metricsDropRate.Observe(float64(drop))
			m := dto.Metric{}
			_ = metricsInvalid.Write(&m)
			invalidInPeriod := m.GetCounter().GetValue() - invalid
			metricsInvalidRate.Observe(invalidInPeriod)
			invalid = m.GetCounter().GetValue()
			logger.Infof("Metrics invalid=%0.2f, receive=%d, delay=%d, drop=%d",
				invalidInPeriod, receive, delay, drop)
			receive = 0
			delay = 0
			drop = 0

			if len(delayedBuffer) > 0 {
				logger.Infof("Delayed(in memory cache) metrics=%d", len(delayedBuffer))
				if buf.len() < maxMetrics {
					toSend := 0
					canSend := maxMetrics - buf.len()
					if canSend > len(delayedBuffer) {
						toSend = len(delayedBuffer)
					} else {
						toSend = canSend
					}
					for _, metric := range delayedBuffer[:toSend] {
						if logTrace {
							logger.Debugf("Cache to add to send mertic=%q", string(metric))
						}
						buf.add(metric)
					}
					// There will be no memory leak here,
					// because every time append add elements to the end of a slice,
					// a new slice will allocated and overwrite the old one.
					delayedBuffer = delayedBuffer[toSend:]
					logger.Debugf("Delayed (left after add to send) metrics=%d", len(delayedBuffer))
				}
			}

			if buf.len() == 0 {
				logger.Info("sendTicker: nothing to receive")
				continue
			}
			go s.send(buf)
			buf = s.store.getBuffer()
		}
	}
}

// send handle delivery of metrics
func (s *Sender) send(buf *senderBuffer) {
	graphiteServers := s.config.Graphite.Servers
	sendInterval := s.config.Sender.SendInterval

	var wg sync.WaitGroup
	for _, gServer := range graphiteServers {
		wg.Add(1)
		go func(server string, w *sync.WaitGroup) {
			sendLogger := log.With(logger, log.String("host", server))
			gConn := s.createGraphiteConn(server, sendInterval)
			if gConn != nil {
				sendLogger.Info("Sending metric")
				err = sendToGraphite(server, gConn, buf)
				if err != nil {
					sendLogger.Errorf("Failed to send, cache to disk: %v", err)
					if err = s.addToDiskCache(buf, server); err != nil {
						sendLogger.Errorf("Failed to cache data to disk: %v", err)
					}
				} else {
					metricsLastSuccessSend.SetToCurrentTime()
					sendLogger.Infof("Success sent metrics=%d", buf.len())
				}
			} else {
				sendLogger.Info("Caching data to disk")
				if err = s.addToDiskCache(buf, server); err != nil {
					sendLogger.Errorf("Failed to cache data to disk: %v", err)
				}
			}
			w.Done()
		}(gServer, &wg)
	}
	wg.Wait()
	s.store.returnToQueue(buf)
}

// launchServer start tcp server
func (s *Sender) launchServer() {
	port := s.config.Sender.Port

	logger.Infof("Listening on :%d", port)
	ln, err := net.Listen("tcp", ":"+strconv.Itoa(port))
	if err != nil {
		logger.Fatalf("Listen failed: %v", err)
	}
	defer func() { _ = ln.Close() }()

	for {
		conn, err := ln.Accept()
		if err != nil {
			logger.Errorf("Accept failed: %v", err)
			continue
		}
		go s.handleConnection(conn)
	}
}

// handleConnection handles incoming client connections
func (s *Sender) handleConnection(conn net.Conn) {
	defer metricsDuration("receive", time.Now())
	logConn := log.With(logger, log.String("host", conn.RemoteAddr().String()))
	if logTrace {
		logConn.Debug("Incomming connection")
	}

	defer func() {
		if logTrace {
			logConn.Debug("Closing incomming connection")
		}
		_ = conn.Close()
	}()
	reader := bufio.NewReader(conn)
OuterLoop:
	for {
		//err := conn.SetReadDeadline(time.Now().Add(10 * time.Second))
		//if err != nil {
		//	log.Error("SetReadDeadline failed", zap.Error(err))
		//	break OuterLoop
		//}

		metric, err := reader.ReadBytes('\n')
		if err != nil {
			if netErr, ok := err.(net.Error); ok && netErr.Timeout() {
				logConn.Debug("Read from tcp socket timed out.")
				break OuterLoop
			}
			switch err {
			case io.EOF:
				break OuterLoop
			default:
				logConn.Errorf("Got an error: %v", err)
				break OuterLoop
			}
		}

		metric, err = validateMetric(metric)
		if err != nil {
			metricsInvalid.Inc()
			logConn.Errorf("validate error metric=%q: %v", string(metric), err)
			continue OuterLoop
		}
		receiveMetricCh <- metric
	}
}

// validateMetric checks if a metric string matches a regex format
// It returns a metric string on success
func validateMetric(metric []byte) ([]byte, error) {
	if len(metric) < 8 { // metricRegex expect line with at last 8 chars
		return metric, fmt.Errorf("skip garbage line, is too short: %d", len(metric))
	}
	matchSubexp := metricRegex.FindSubmatch(metric)
	if len(matchSubexp) == 0 {
		return metric, fmt.Errorf("data does not match regexp %s", metricRegex.String())
	}
	if len(matchSubexp[1]) == 0 { // timestamp not present
		//logTracef("Metric without timestamp %s", metric)
		timestamp := strconv.Itoa(int(time.Now().Unix()))

		metric[len(metric)-1] = ' ' // replace \n with space
		metric = append(metric, []byte(timestamp)...)
		metric = append(metric, '\n')
		return metric, nil
	}
	return metric, nil
}

func setupLogger(config *Config) log.Logger {
	l, err := log.ParseLevel(config.Sender.LogLevel)
	if err != nil {
		panic(err)
	}
	cfg := zap.KVConfig(l)

	output := config.Sender.LogFile
	if !strings.HasPrefix(output, "std") {
		err := logrotate.RegisterLogrotateSink(syscall.SIGHUP)
		if err != nil {
			panic(err)
		}
		logPath, err := filepath.Abs(config.Sender.LogFile)
		if err != nil {
			panic(err)
		}
		output = "logrotate://" + logPath
	}

	cfg.OutputPaths = []string{output}
	return zap.Must(cfg)
}

var logTrace bool

// Run run all sender related goroutines
// and perform initial send cached data
func Run(args *Arguments) error {
	//prepare
	logTrace = args.LogTrace
	cfg, err := loadConfig(args)
	if err != nil {
		return err
	}
	logger = setupLogger(cfg)
	logger.Debugf("Sender config: %v", cfg)
	sender := NewSender(cfg)

	// startup
	go sender.startMonitoringExporter()
	go sender.run()
	go sender.launchServer()
	sender.sendCachedData()
	return nil
}
