#include "master_config.h"

#include <tools/clustermaster/common/util.h>

#include <util/folder/dirut.h>
#include <util/generic/ptr.h>
#include <util/random/random.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/string/strip.h>
#include <util/system/shellcommand.h>
#include <util/system/tempfile.h>

static void ReadFromFile(TString& output, const TMasterConfigSource& configSource, int reclevel, int maxrecursion, TVector<TString>& precmds, TVector<TString>& filters) {
    if (maxrecursion < 0)
        ythrow yexception() << "Max recursion level for included files reached";

    THolder<IInputStream> input;

    TFile file;
    if (configSource.IsPath()) {
        TFile file(configSource.GetPath().GetPath(), OpenExisting | RdOnly);
        output.reserve(output.size() + file.GetLength());
        input.Reset(new TFileInput(file));
    } else {
        output.reserve(output.size() + configSource.GetContent().size());
        input.Reset(new TStringInput(configSource.GetContent()));
    }

    bool includes = false;
    int line = 0;
    TString lineRead;

    size_t read;
    while (read = input->ReadTo(lineRead, '\n')) {
        TString lineStripped = lineRead;
        StripInPlace(lineStripped);
        ++line;

        bool appendLine = true;
        bool comment = (*lineStripped.begin() == '#');

        if (includes || comment) {
            TStringBuf first, rest;
            SplitByOneOf(lineStripped, " \t", first, rest);

            if (includes && (first == "." || first == "source")) {
                TString includedPath = TString(rest);

                if (*includedPath.begin() != '/' && *includedPath.begin() != '~') {
                    // for local paths, globalize based on parent script
                    includedPath = GetDirName(configSource.GetPath().GetPath()) + "/" + includedPath;
                }

                try {
                    ReadFromFile(output, TMasterConfigSource(TFsPath(includedPath)), reclevel+1, maxrecursion-1, precmds, filters);
                } catch (const yexception &e) {
                    ythrow yexception() << "Cannot process file " << includedPath << " included from " << configSource.GetPath() << " in line " << line << ": " << e.what();
                }

                appendLine = false;
            } else if (comment && first == "#!+INCLUDES") {
                includes = true;
                appendLine = false;
            } else if (comment && first == "#!-INCLUDES") {
                includes = false;
                appendLine = false;
            } else if (comment && first == "#!PRECMD") {
                precmds.push_back(TString(rest));
                appendLine = false;
            } else if (comment && first == "#!FILTER") {
                if (reclevel != 0)
                    ythrow yexception() << "Filter encountered in included file " << configSource.GetPath() << " (level " << reclevel << "), but are only allowed on top-level";

                filters.push_back(TString(rest));
                appendLine = false;
            }
        }

        if (appendLine) {
            output.append(lineRead);
            if (read == (lineRead.length() + 1)) { // it means that last symbol was '\n'
                output.append("\n");
            }
        }
    }
}



TAutoPtr<TConfigMessage> ParseMasterConfig(const TMasterConfigSource& configSource, const TString& masterHost, TIpPort masterHttpPort) {
    TAutoPtr<TConfigMessage> r(new TConfigMessage());

    r->SetMasterHost(masterHost);
    r->SetMasterHttpPort(masterHttpPort);

    TVector<TString> precmds;
    TVector<TString> filters;

    TString contents;
    ReadFromFile(contents, configSource, 0, 16, precmds, filters);
    TString configDir;
    if (configSource.IsPath()) {
        configDir = configSource.GetPath().RealPath().Dirname();
    }

    // Precmds not empty -> run them...
    if (!precmds.empty()) {
        contents.clear();
        filters.clear();

        for (TVector<TString>::iterator precmd = precmds.begin(); precmd != precmds.end(); ++precmd) {
            int ret = system(precmd->data());
            if (ret != 0)
                ythrow yexception() << "Precmd \"" << *precmd << "\" failed with code " << ret << ", see master log for stdout/stderr contents";
        }

        // ...and reread config (as it may be changed by them, e.g. svn up)
        ReadFromFile(contents, configSource, 0, 16, precmds, filters);
        // this time precmds are ignored
    }

    // Run config through filters
    for (TVector<TString>::iterator filter = filters.begin(); filter != filters.end(); ++filter) {
        TTempFile configFile(TString("/tmp/clustermaster-config-") + ToString(MicroSeconds()) + ToString(RandomNumber<ui64>()));

        TString filterCommand;
        TString filterStderr;
        try {
            if (filter->StartsWith("./")) {
                Y_VERIFY(configSource.IsPath(), "Cannot use relative paths when script is read from stream.");
                TStringBuf commandRelativePath;
                TStringBuf(*filter).AfterPrefix("./", commandRelativePath);
                filterCommand = configDir + "/" + TString{commandRelativePath};
            } else {
                filterCommand = *filter;
            }

            TShellCommandOptions commandOptions;
            TStringInput configStringInput(contents);
            TStringOutput filterStderrStream(filterStderr);

            commandOptions.SetInputStream(&configStringInput);
            commandOptions.SetErrorStream(&filterStderrStream);

            TShellCommand filterProcess(filterCommand + " > " + configFile.Name(), commandOptions);
            filterProcess.Run();
            if (filterProcess.GetExitCode().GetRef()) {
                ythrow yexception() << filterStderr;
            }

            contents = TUnbufferedFileInput(configFile.Name()).ReadAll();
        } catch (const std::exception& e) {
            ythrow yexception() << "Error while executing script filter \"" << filterCommand << "\":\n" << e.what();
        }
    }

    r->SetConfig(contents);

    return r;
}

