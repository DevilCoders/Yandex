#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/snippets/strhl/goodwrds.h>
#include <kernel/snippets/strhl/hilite_mark.h>
#include <kernel/snippets/urlcut/url_wanderer.h>
#include <kernel/snippets/urlcut/urlcut.h>
#include <kernel/snippets/urlmenu/cut/cut.h>
#include <kernel/snippets/urlmenu/dump/dump.h>
#include <kernel/snippets/urlmenu/searcher/searcher.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/file.h>
#include <util/string/util.h>
#include <util/string/vector.h>


static void ProcessSearchQueries(const TString& searchFile, const TString& searchUrl, const TString& query) {
    NUrlMenu::TSearcher searcher(searchFile);
    TUrlMenuVector arcMenu;
    TString url = searchUrl;
    if (! url.empty()) {
        if (searcher.Search(url, arcMenu)) {
            TCreateTreeOptions opts(LI_DEFAULT_REQUEST_LANGUAGES);
            TUtf16String wideQuery = UTF8ToWide(query);
            TRichNodePtr richNode = CreateRichNode(wideQuery, opts);
            TInlineHighlighter inlineHighlighter;
            inlineHighlighter.AddRequest(*richNode.Get());
            NUrlCutter::TRichTreeWanderer rTreeWanderer(CreateRichTree(wideQuery, opts));
            NUrlMenu::THilitedUrlMenu hilitedUrlMenu(arcMenu);
            hilitedUrlMenu.HiliteAndCut(76, rTreeWanderer, inlineHighlighter);
            arcMenu = hilitedUrlMenu.Merge(DEFAULT_MARKS.OpenTag, DEFAULT_MARKS.CloseTag);
            Cout << NUrlMenu::Serialize(arcMenu) << Endl;
        } else
            Cerr << "can't define urlmenu for '" << url << "'" << Endl;
    } else {
        while (Cin.ReadLine(url)) {
            if (searcher.Search(url, arcMenu))
                Cout << NUrlMenu::Serialize(arcMenu) << Endl;
            else
                Cerr << "can't define urlmenu for '" << url << "'" << Endl;
        }
    }
}

static void ProcessUrlCutting(const TString& urlPart, const TString& query, const size_t length) {
    TCreateTreeOptions opts(LI_DEFAULT_REQUEST_LANGUAGES);
    NUrlCutter::TRichTreeWanderer rTreeWanderer(CreateRichTree(UTF8ToWide(query), opts));
    NUrlCutter::THilitedString hilitedTail =
        NUrlCutter::HiliteAndCutUrlMenuPath(urlPart, length, rTreeWanderer, LANG_RUS);
    Cout << hilitedTail.Merge(DEFAULT_MARKS.OpenTag, DEFAULT_MARKS.CloseTag) << Endl;
}

static void PrintSearchIndex(const TString& searchFile, const TString& outputFile) {
    THolder<TFixedBufferFileOutput> out_fd;
    if (! outputFile.empty())
        out_fd.Reset(new TFixedBufferFileOutput(outputFile));
    NUrlMenu::TSearcher(searchFile).Print(out_fd.Get() ? out_fd.Get() : &Cout);
}

int main(int argc, const char* argv[]) {
    try {
        NLastGetopt::TOpts opts;
        opts.AddCharOption('I', "index mode").NoArgument();
        opts.AddCharOption('P', "print mode").NoArgument();
        opts.AddCharOption('S', "search mode").NoArgument();
        opts.AddCharOption('T', "tail cutting mode").NoArgument();

        opts.AddCharOption('m', "minimize output trie (in index mode)").NoArgument();
        opts.AddCharOption('v', "be verbose (in index mode)").NoArgument();
        opts.AddLongOption("sorted-input", "input is already sorted (in index mode)").NoArgument();
        opts.AddLongOption('n', "normalized-input", "input urls is already normalized (in index mode)").NoArgument();

        opts.AddCharOption('i', "input file (in index, print, search modes)").RequiredArgument("FILE").DefaultValue("");
        opts.AddCharOption('o', "output file (in index, print modes)").RequiredArgument("FILE").DefaultValue("");
        opts.AddCharOption('u', "searched url (in search mode) or url tail (in tail cutting mode)").RequiredArgument("URL").DefaultValue("");
        opts.AddCharOption('q', "query for tail cutting (in search, tail cutting modes)").RequiredArgument("QUERY").DefaultValue("");
        opts.AddCharOption('s', "cutting length (in tail cutting mode)").RequiredArgument("N").DefaultValue("0");

        NLastGetopt::TOptsParseResult optsParseResult(&opts, argc, argv);
        bool indexMode = optsParseResult.Has('I');
        bool printMode = optsParseResult.Has('P');
        bool searchMode = optsParseResult.Has('S');
        bool tailMode = optsParseResult.Has('T');

        bool noErrors = true;
        noErrors &= (indexMode || printMode || searchMode) == optsParseResult.Has('i');
        noErrors &= indexMode ? indexMode == optsParseResult.Has('o') : true;
        noErrors &= (indexMode ? 1 : 0) + (printMode ? 1 : 0) + (searchMode ? 1 : 0) + (tailMode ? 1 : 0) == 1;

        if (!noErrors) {
            opts.PrintUsage(optsParseResult.GetProgramName());
            return 1;
        }

        const char* inputFile = optsParseResult.Get('i');
        const char* outputFile = optsParseResult.Get('o');
        const char* urlPart = optsParseResult.Get('u');
        const char* query = optsParseResult.Get('q');
        size_t length = optsParseResult.Get<size_t>('s');

        bool minimized = optsParseResult.Has('m');
        bool verbose = optsParseResult.Has('v');
        bool sorted = optsParseResult.Has("sorted-input");
        bool normalized = optsParseResult.Has('n');

        if (indexMode) {
            NUrlMenu::TSearcherIndexCreationOptions creationOptions;
            creationOptions.Minimize = minimized;
            creationOptions.Verbose = verbose;
            creationOptions.Sorted = sorted;
            creationOptions.Normalize = !normalized;

            NUrlMenu::CreateSearcherIndex(inputFile, outputFile, creationOptions);
        } else if (printMode)
            PrintSearchIndex(inputFile, outputFile);
        else if (searchMode)
            ProcessSearchQueries(inputFile, urlPart, query);
        else if (tailMode)
            ProcessUrlCutting(urlPart, query, length);

        return 0;
    } catch (...) {
        Cerr << "urlmenuindexer error: " << CurrentExceptionMessage() << Endl;
        return 1;
    }

}
