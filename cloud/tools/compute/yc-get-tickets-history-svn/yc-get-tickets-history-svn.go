package main

import (
	"bufio"
	"bytes"
	"errors"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"os/exec"
	"os/user"
	"path/filepath"
	"regexp"
	"sort"
	"strconv"
	"strings"
)

var (
	ordersP      = flag.String("order", "CLOUD", "tracker orders, which need to find. Comma separated.")
	projectP     = flag.String("project", "", "path to projects, relative to arcadia. Example: cloud/compute/snapshot")
	revisionsP   = flag.String("revisions", "", "SVN revisions for check in form start[..last]")
	svnCommandP  = flag.String("svn-command", "/usr/bin/svn", "command for run svn command line util")
	svnRepoP     = flag.String("repo", "svn+ssh://arcadia.yandex.ru/arc", "svn repo path")
	svnPrefix    = flag.String("repo-prefix", "trunk/arcadia", "path to arcadia trunk from repo root")
	tokenPathP   = flag.String("tokenPath", "", "Path to OAuth token. Default: ~/.arc/token")
	outputFormat = flag.String("outformal", "ticket-id", "Accepted: ticket-id, ticket-link")
	prevPlusOne  = flag.Bool("prev-plus-one", false, "Add one to start revision number")
)

func main() {
	flag.Parse()

	if *projectP == "" {
		log.Fatalf("Need project path")
	}

	if *tokenPathP == "" {
		osUser, err := user.Current()
		if err != nil {
			log.Fatalf("Can't get current user: %v", err)
		}
		*tokenPathP = filepath.Join(osUser.HomeDir, ".arc", "token")
	}

	tokenBytes, err := ioutil.ReadFile(*tokenPathP)
	if err != nil {
		log.Fatalf("Can't read token file '%v': %v\nhttps://a.yandex-team.ru/api/token?code=9767177", *tokenPathP, err)
	}
	token := strings.TrimSpace(string(tokenBytes))
	_ = token

	if *revisionsP == "" {
		log.Fatal("need --revisions argument")
	}

	first, last, err := getRevisions(*revisionsP)
	if err != nil {
		log.Fatalf("Can't parse revisions: %v", err)
	}

	var revisions string
	if last == "" {
		revisions = fmt.Sprintf("%v:HEAD", first)
	} else {
		revisions = fmt.Sprintf("%v:%v", first, last)
	}

	stdout := &bytes.Buffer{}
	stderr := &bytes.Buffer{}
	url := fmt.Sprintf("%v/%v/%v", *svnRepoP, *svnPrefix, *projectP)
	cmd := exec.Command(*svnCommandP, "log", "-r", revisions, url)
	cmd.Stdout = stdout
	cmd.Stderr = stderr
	err = cmd.Run()
	if err != nil {
		log.Fatalf("Can't get svn revisions: %v\n%s\n%s\n", err, stdout, stderr)
	}

	tasks := make(map[string]struct{})

	orders := strings.FieldsFunc(*ordersP, func(r rune) bool {
		return r == ','
	})

	commitsText := stdout.Bytes()
	reviews := extractReviews(commitsText)
	parseTasks(tasks, commitsText, orders)

	for _, review := range reviews {
		respBytes := getReview(review, token)
		parseTasks(tasks, respBytes, orders)
	}
	var taskTickets = make([]string, 0, len(tasks))
	for task := range tasks {
		taskTickets = append(taskTickets, task)
	}
	sort.Strings(taskTickets)
	printTickets(taskTickets)
}

func printTickets(tickets []string) {
	for _, t := range tickets {
		switch *outputFormat {
		case "ticket-id":
			fmt.Println(t)
		case "ticket-link":
			fmt.Println("https://st.yandex-team.ru/" + t)
		default:
			log.Fatalf("Unknown out format: '%v'\n", *outputFormat)
		}
	}
}

func getReview(review string, token string) []byte {
	url := "https://a.yandex-team.ru/api/review/review-request/" + review
	req, err := http.NewRequest(http.MethodGet, url, nil)
	if err != nil {
		log.Fatalf("Can't create arc request: %v\n", err)
	}
	req.Header.Set("Authorization", "OAuth "+token)
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		log.Fatalf("Error while request arcanum for review '%v': %v\n", review, err)
	}
	if resp.StatusCode != http.StatusOK {
		log.Fatalf("Bad revirew ask status code for review '%v': %v\n", review, resp.StatusCode)
	}
	respBytes, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		log.Fatalf("Can't read all for review '%v': %v\n", review, err)
	}
	_ = resp.Body.Close()
	return respBytes
}

func getRevisions(s string) (first, last string, err error) {
	var revisionParts = strings.Split(s, "..")
	if len(revisionParts) > 2 || len(revisionParts) == 0 {
		return "", "", errors.New("bad format revisions")
	}

	if revisionParts[0] != "" {
		first = revisionParts[0]
		if *prevPlusOne {
			firstNum, err := strconv.Atoi(first)
			if err != nil {
				return "", "", fmt.Errorf("can't parse first revision: %v", err)
			}
			first = strconv.Itoa(firstNum + 1)
		}
	}

	if len(revisionParts) > 1 && revisionParts[1] != "" {
		last = revisionParts[1]
	}

	return first, last, nil
}

func extractReviews(s []byte) []string {
	var res []string
	scanner := bufio.NewScanner(bytes.NewReader(s))
	for scanner.Scan() {
		line := scanner.Text()
		line = strings.TrimSpace(line)
		if strings.HasPrefix(line, "REVIEW: ") {
			review := line[len("REVIEW: "):]
			res = append(res, review)
		}
	}
	return res
}

func parseTasks(m map[string]struct{}, textBytes []byte, orders []string) {
	var regexps = make([]*regexp.Regexp, 0, len(orders))
	for _, order := range orders {
		r := regexp.MustCompile("(?i)" + order + `-\d+`)
		regexps = append(regexps, r)
	}

	text := string(textBytes)
	for _, r := range regexps {
		matches := r.FindAllString(text, -1)
		for _, match := range matches {
			m[match] = struct{}{}
		}
	}
}
