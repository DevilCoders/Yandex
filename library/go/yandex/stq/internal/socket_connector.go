package internal

import (
	"encoding/binary"
	"fmt"
	"io"
	"net"
	"os"
	"strconv"
	"time"

	"go.mongodb.org/mongo-driver/bson"

	"a.yandex-team.ru/library/go/yandex/stq/pkg/worker"
)

type StqRunnerSocketConnector struct {
	conn net.Conn
}

func getConnection(fd int) (net.Conn, error) {
	filePipe := os.NewFile(uintptr(fd), "LISTEN_FD_"+strconv.Itoa(fd))
	defer filePipe.Close()

	conn, err := net.FileConn(filePipe)
	if err != nil {
		return nil, err
	}

	return conn, err
}

func NewSocketStqRunnerConnector(fd int) (*StqRunnerSocketConnector, error) {
	conn, err := getConnection(fd)
	if err != nil {
		return nil, fmt.Errorf("cant get connection: %w", err)
	}

	runnerConnector := StqRunnerSocketConnector{
		conn: conn,
	}

	return &runnerConnector, nil
}

func (connector *StqRunnerSocketConnector) GetRequest() (worker.Request, error) {
	rawDataSize := make([]byte, 4)

	readTimeout := 300 * time.Millisecond
	err := connector.conn.SetReadDeadline(time.Now().Add(readTimeout))
	if err != nil {
		return worker.Request{}, fmt.Errorf("cant set read deadline: %w", err)
	}

	_, err = io.ReadFull(connector.conn, rawDataSize)
	if err != nil {
		return worker.Request{}, fmt.Errorf("cant read size: %w", err)
	}
	dataSize := binary.LittleEndian.Uint16(rawDataSize)

	err = connector.conn.SetReadDeadline(time.Time{})
	if err != nil {
		return worker.Request{}, fmt.Errorf("cant unset read deadline: %w", err)
	}
	rawData := make([]byte, dataSize)
	_, err = io.ReadFull(connector.conn, rawData)
	if err != nil {
		return worker.Request{}, fmt.Errorf("cant read data: %w", err)
	}

	var workerRequest worker.Request
	err = bson.Unmarshal(rawData, &workerRequest)
	if err != nil {
		return worker.Request{}, fmt.Errorf("cant unmarshal bson worker request: %w", err)
	}

	return workerRequest, nil
}

func (connector *StqRunnerSocketConnector) SendResponse(response worker.Response) error {
	rawData, err := bson.Marshal(response)
	if err != nil {
		return fmt.Errorf("cant marshal bson worker response: %w", err)
	}

	dataSize := len(rawData)
	rawDataSize := make([]byte, 4)
	binary.LittleEndian.PutUint16(rawDataSize, uint16(dataSize))

	_, err = connector.conn.Write(rawDataSize)
	if err != nil {
		return fmt.Errorf("cant write size: %w", err)
	}

	_, err = connector.conn.Write(rawData)
	if err != nil {
		return fmt.Errorf("cant write data: %w", err)
	}

	return nil
}
