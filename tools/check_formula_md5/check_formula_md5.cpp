#include <library/cpp/getopt/small/last_getopt.h>

#include <library/cpp/digest/md5/md5.h>
#include <util/stream/file.h>

int main(int argc, char* argv[]) {
    TString formulaFile;
    TString md5File;

    NLastGetopt::TOpts opts;
    opts.AddLongOption('f', "formula_file",
        "File containing formula.")
        .Optional()
        .Required()
        .StoreResult(&formulaFile);
    opts.AddLongOption('m', "md5_file",
        "File containing md5.")
        .Optional()
        .Required()
        .StoreResult(&md5File);
    const NLastGetopt::TOptsParseResult optsres(&opts, argc, argv);

    char md5buf[33];
    if (!MD5::File(formulaFile.data(), md5buf)) {
        ythrow yexception() << "Can't calculate md5 of " << formulaFile;
    }

    TFileInput in(md5File);

    TString md5;
    if (!in.ReadLine(md5))
        ythrow yexception() << md5File << " has wrong format";

    if (md5 != TString(md5buf))
        ythrow yexception() << "md5 of " << formulaFile << " is " << md5buf << " while " << md5 << " recorded in " << md5File;
}
