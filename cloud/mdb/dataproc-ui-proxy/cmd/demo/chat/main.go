package main

import (
	"bufio"
	"context"
	"crypto/tls"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"net"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/agent"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/common"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/server"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/util"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

var proxyAddr = flag.String("addr", "127.0.0.1:8080", "http service address")

func main() {
	agentID := "default"

	tlsCert, tlsConfig, err := common.GenerateSelfSignedCert([]string{})
	if err != nil {
		fmt.Println("Failed to generate tls key pair:", err)
		os.Exit(1)
	}

	ctx := context.Background()
	logger := createLogger()
	proxyManagementAddr := getFreeAddr()
	chatServerAddr := getFreeAddr()
	arcRoot := getArcRoot()
	logger.Infof("Arc root is: %s", arcRoot)
	chatPath := filepath.Join(arcRoot, "vendor/github.com/gorilla/websocket/examples/chat/")
	logger.Infof("Building %s ...", chatPath)
	compile(chatPath)

	var chatServerCtx context.Context
	var stopChatServer context.CancelFunc
	startChatServer := func() {
		chatServerCtx, stopChatServer = context.WithCancel(ctx)
		go runChatServer(chatServerCtx, chatPath, logger, chatServerAddr)
	}
	startChatServer()

	proxyServer := server.NewProxy(server.DefaultConfig(), logger)
	go runAdapterFacingServer(proxyServer, proxyManagementAddr, tlsConfig)
	go runProxyServer(proxyServer, agentID, *proxyAddr, logger)

	var agentCtx context.Context
	var stopAgent context.CancelFunc
	startAgent := func() {
		agentCtx, stopAgent = context.WithCancel(ctx)
		go runAgent(agentCtx, logger, tlsCert, proxyManagementAddr, agentID, chatServerAddr)
	}
	startAgent()

	reader := bufio.NewReader(os.Stdin)
	for {
		str, err := reader.ReadString('\n')
		if err != nil {
			fmt.Println("Failed to read from stdin:", err)
			os.Exit(1)
		}
		str = strings.TrimSpace(str)
		switch str {
		case "stopagent":
			logger.Infof("Stopping agent ...")
			stopAgent()
		case "startagent":
			logger.Infof("Starting agent ...")
			startAgent()
		case "stopchat":
			logger.Infof("Stopping chat server ...")
			stopChatServer()
		case "startchat":
			logger.Infof("Starting chat server ...")
			startChatServer()
		default:
			logger.Infof("Unknown command: %q", str)
		}
	}
}

func getFreeAddr() string {
	listener, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		fmt.Println("Failed to find open port:", err)
		os.Exit(1)
	}
	addr := listener.Addr().String()
	err = listener.Close()
	if err != nil {
		fmt.Println("Failed to close listener:", err)
		os.Exit(1)
	}
	return addr
}

func createLogger() log.Logger {
	zapConfig := zap.KVConfig(log.DebugLevel)
	zapConfig.OutputPaths = []string{"stdout"}
	logger, err := zap.New(zapConfig)
	if err != nil {
		fmt.Println("Failed to create logger:", err)
		os.Exit(1)
	}
	return logger
}

func getArcRoot() string {
	out, err := exec.Command("arc", "root").Output()
	if err != nil {
		fmt.Println("Failed to find out arc root:", err)
		os.Exit(1)
	}
	return strings.TrimSpace(string(out))
}

func compile(path string) {
	cmd := exec.Command("ya", "make", path)
	err := cmd.Run()
	if err != nil {
		fmt.Println("Failed to compile:", err)
		os.Exit(1)
	}
}

func runChatServer(ctx context.Context, path string, logger log.Logger, addr string) {
	chatExecutablePath := filepath.Join(path, "chat")
	cmd := exec.Command(chatExecutablePath, "-addr", addr)
	go func() {
		<-ctx.Done()
		err := cmd.Process.Kill()
		if err != nil {
			logger.Errorf("Failed to kill chat server: %s", err)
		}
	}()
	cmd.Dir = path
	stdout, _ := cmd.StdoutPipe()
	cmd.Stderr = cmd.Stdout
	err := cmd.Start()
	if err != nil {
		fmt.Println("Failed to start char server:", err)
		os.Exit(1)
	}
	logger.Infof("Chat server started!")
	buffer := make([]byte, 1024)
	for {
		n, err := stdout.Read(buffer)
		if err != nil {
			if err == io.EOF {
				break
			}
			fmt.Println("Failed to read chat-server executable output:", err)
			os.Exit(1)
		}
		logger.Infof("Chat server output: %s", buffer[:n])
	}
	_ = cmd.Wait()
}

func runAdapterFacingServer(proxyServer *server.Server, addr string, tlsConfig *tls.Config) {
	tlsConfig.MinVersion = tls.VersionTLS12

	httpServer := &http.Server{
		Addr:      addr,
		Handler:   proxyServer.AgentRequestHandler(),
		TLSConfig: tlsConfig,
	}
	err := httpServer.ListenAndServeTLS("", "")
	if err != nil {
		fmt.Println("Adapter facing server exited:", err)
		os.Exit(1)
	}
}

func runProxyServer(proxyServer *server.Server, agentID string, addr string, logger log.Logger) {
	userHandler := http.HandlerFunc(func(writer http.ResponseWriter, request *http.Request) {
		request = request.WithContext(util.WithClusterID(request.Context(), agentID))
		proxyServer.AuthenticatedUserRequestHandler().ServeHTTP(writer, request)
	})

	proxyURL := "http://" + addr
	logger.Infof("Proxy url: %s", proxyURL)
	err := http.ListenAndServe(addr, userHandler)
	if err != nil {
		fmt.Println("Failed to run user facing proxy server:", err)
		os.Exit(1)
	}
}

func runAgent(ctx context.Context, logger log.Logger, cert []byte, proxyAddr, agentID, backendAddr string) {
	upstreamURL := backendAddr
	if strings.HasPrefix(backendAddr, ":") {
		upstreamURL = "http://127.0.0.1" + backendAddr
	} else {
		upstreamURL = "http://" + backendAddr
	}

	tmpfile, err := ioutil.TempFile("", "*.crt")
	if err != nil {
		fmt.Println("Failed to create agent:", err)
		os.Exit(1)
	}

	_, err = tmpfile.Write(cert)
	if err != nil {
		fmt.Println("Failed to create agent:", err)
		os.Exit(1)
	}
	err = tmpfile.Close()
	if err != nil {
		fmt.Println("Failed to create agent:", err)
		os.Exit(1)
	}

	config := agent.Config{
		ProxyServerURL:     "https://" + proxyAddr + "/",
		ProxyServerTimeout: 60 * time.Second,
		CAFile:             tmpfile.Name(),
		UpstreamURL:        upstreamURL,
		AgentID:            agentID,
		ForwardUserID:      false,
		WebsocketShimPath:  "/ws",
	}
	proxyAgent, err := agent.New(config, nil, logger, nil)
	_ = os.Remove(tmpfile.Name())
	if err != nil {
		fmt.Println("Failed to create agent:", err)
		os.Exit(1)
	}
	proxyAgent.Run(ctx)
}
