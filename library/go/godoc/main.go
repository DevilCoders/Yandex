package main

import (
	"errors"
	"flag"
	"fmt"
	"log"
	"net"
	"net/http"
	"os"
	"path/filepath"
	"strings"
	"text/template"
	"time"

	"golang.org/x/tools/godoc"
	"golang.org/x/tools/godoc/static"
	"golang.org/x/tools/godoc/vfs"
	"golang.org/x/tools/godoc/vfs/mapfs"

	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb"
	"a.yandex-team.ru/library/go/yandex/tvm/tvmtool"
	"a.yandex-team.ru/library/go/yatool"
)

var fs vfs.NameSpace
var pres *godoc.Presentation

func readTemplate(name string) *template.Template {
	if pres == nil {
		panic("no global Presentation set yet")
	}
	path := "lib/godoc/" + name

	// use underlying file system fs to read the template file
	// (cannot use template ParseFile functions directly)
	data, err := vfs.ReadFile(fs, path)
	if err != nil {
		log.Fatal("readTemplate: ", err)
	}
	// be explicit with errors (for app engine use)
	t, err := template.New(name).Funcs(pres.FuncMap()).Parse(string(data))
	if err != nil {
		log.Fatal("readTemplate: ", err)
	}
	return t
}

func readTemplates(p *godoc.Presentation) {
	//codewalkHTML = readTemplate("codewalk.html")
	//codewalkdirHTML = readTemplate("codewalkdir.html")
	p.CallGraphHTML = readTemplate("callgraph.html")
	p.DirlistHTML = readTemplate("dirlist.html")
	p.ErrorHTML = readTemplate("error.html")
	p.ExampleHTML = readTemplate("example.html")
	p.GodocHTML = readTemplate("godoc.html")
	p.ImplementsHTML = readTemplate("implements.html")
	p.MethodSetHTML = readTemplate("methodset.html")
	p.PackageHTML = readTemplate("package.html")
	p.PackageRootHTML = readTemplate("packageroot.html")
	p.SearchHTML = readTemplate("search.html")
	p.SearchDocHTML = readTemplate("searchdoc.html")
	p.SearchCodeHTML = readTemplate("searchcode.html")
	p.SearchTxtHTML = readTemplate("searchtxt.html")
	//p.SearchDescXML = readTemplate("opensearch.xml")
}

func arcanumURLForSrc(src string) string {
	if strings.HasPrefix(src, "/src/a.yandex-team.ru") {
		return "https://a.yandex-team.ru/arc/trunk/arcadia" + strings.TrimPrefix(src, "/src/a.yandex-team.ru")
	} else if strings.HasPrefix(src, "/src") {
		return "https://a.yandex-team.ru/arc/trunk/arcadia/vendor" + strings.TrimPrefix(src, "/src")
	} else if strings.HasPrefix(src, "a.yandex-team.ru/") {
		return "https://a.yandex-team.ru/arc/trunk/arcadia" + strings.TrimPrefix(src, "a.yandex-team.ru")
	} else {
		return "https://a.yandex-team.ru/arc/trunk/arcadia/vendor/" + src
	}
}

func arcanumURLForSrcPos(src string, line, low, high int) string {
	url := arcanumURLForSrc(src)
	return url + fmt.Sprintf("#L%d", line)
}

func checkArcadiaStaleness(vfs vfs.NameSpace) error {
	topDirs, err := vfs.ReadDir("/src/a.yandex-team.ru")
	if err != nil {
		return err
	}

	if len(topDirs) == 0 {
		return errors.New("arcadia directory is empty")
	}

	var maxTime time.Time
	for _, dir := range topDirs {
		if dir.ModTime().After(maxTime) {
			maxTime = dir.ModTime()
		}
	}

	if maxTime.Add(12 * time.Hour).Before(time.Now()) {
		return fmt.Errorf("max update time is %v; arcadia is stale", maxTime)
	}

	return nil
}

func bindAll(root vfs.NameSpace, vfsPath, targetPath string) {
	vfsPath = strings.ReplaceAll(vfsPath, string(filepath.Separator), "/")
	parts := strings.Split(vfsPath, "/")
	for _, part := range parts[:len(parts)-1] {
		ns := vfs.NewNameSpace()
		root.Bind("/"+part, ns, "", vfs.BindAfter)
		root = ns
	}

	root.Bind("/"+parts[len(parts)-1], vfs.OS(targetPath), "", vfs.BindAfter)
}

func main() {
	var (
		httpAddress string
		auth        bool
		allArcadia  bool
		pkgRoot     string
	)

	flag.StringVar(&httpAddress, "http", ":8080", "HTTP service address (e.g. ':8080'")
	flag.BoolVar(&auth, "auth", false, "Check authorization using blackbox")
	flag.BoolVar(&allArcadia, "all", false, "Generate documentation for all Arcadia packages")
	flag.Parse()

	arcadiaRoot, err := yatool.ArcadiaRoot()
	if err != nil {
		log.Fatalf("failed to find arcadia root: %v", err)
	}

	goRoot := filepath.Join(arcadiaRoot, "contrib", "go", "_std_1.18")
	vfs.GOROOT = goRoot

	fs = vfs.NameSpace{}
	fs.Bind("/", vfs.OS(goRoot), "/", vfs.BindReplace)
	fs.Bind("/lib/godoc", mapfs.New(static.Files), "/", vfs.BindReplace)

	if allArcadia {
		fs.Bind("/src/a.yandex-team.ru", vfs.OS(arcadiaRoot), "", vfs.BindAfter)
	} else {
		libraryPath := filepath.Join(arcadiaRoot, "library", "go")
		currentRoot, err := filepath.Abs(".")
		if err != nil {
			log.Fatalf("failed to get current path: %v", err)
		}

		projectPath, err := filepath.Rel(arcadiaRoot, currentRoot)
		if err != nil {
			log.Fatalf("failed to get relative project path: %v", err)
		}

		pkgRoot = projectPath
		rootNS := vfs.NewNameSpace()
		bindAll(rootNS, projectPath, currentRoot)
		bindAll(rootNS, "library/go", libraryPath)
		fs.Bind("/src/a.yandex-team.ru", rootNS, "", vfs.BindAfter)
	}

	fs.Bind("/src", vfs.OS(filepath.Join(arcadiaRoot, "vendor")), "/", vfs.BindAfter)

	corpus := godoc.NewCorpus(fs)
	if err := corpus.Init(); err != nil {
		log.Fatal(err)
	}

	pres = godoc.NewPresentation(corpus)
	pres.URLForSrc = arcanumURLForSrc
	pres.URLForSrcPos = arcanumURLForSrcPos

	readTemplates(pres)

	hostname, _ := os.Hostname()
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Add("X-GoDoc-Backend", hostname)

		if auth && !checkAuth(w, r) {
			return
		}

		pres.ServeHTTP(w, r)
	})

	http.HandleFunc("/healthcheck", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Add("X-GoDoc-Backend", hostname)

		if err := checkArcadiaStaleness(fs); err != nil {
			http.Error(w, fmt.Sprintf("%+v", err), 500)
			return
		}

		w.WriteHeader(200)
	})

	listener, err := net.Listen("tcp", httpAddress)
	if err != nil {
		log.Fatalf("cannot listen %q: %s", httpAddress, err)
	}

	fmt.Printf(
		"godoc started at: %s\n",
		listener.Addr().String(),
	)
	fmt.Printf(
		"check your packages at: http://%s/pkg/a.yandex-team.ru/%s\n",
		strings.ReplaceAll(listener.Addr().String(), "[::]", "localhost"),
		pkgRoot,
	)

	if auth {
		tvmClient, err := tvmtool.NewQloudClient()
		if err != nil {
			panic(err)
		}

		blackboxClient, err = httpbb.NewIntranet(
			httpbb.WithLogger(new(nop.Logger)),
			httpbb.WithTVM(tvmClient),
		)
		if err != nil {
			panic(err)
		}
	}

	log.Fatal(http.Serve(listener, nil))
}
