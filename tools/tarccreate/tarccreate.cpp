#include <util/stream/output.h>
#include <util/stream/file.h>
#include <kernel/tarc/iface/arcface.h>

void PrintUsage()
{
    Cerr <<
    "Copyright (c) OOO \"Yandex\". All rights reserved.\n"
    "tarccreate - create empty archive\n"
    "Usage: tarccreate archivepath\n"
    "where archivepath - path to the arc-file to create\n"
    ;
}

void CreateEmptyArchive(const TString& path) {
    TFixedBufferFileOutput archOut(path);
    WriteTextArchiveHeader(archOut);
    archOut.Finish();
}

void main_except_wrap (int argc, char* argv[])
{
    if (argc < 2 ) {
        PrintUsage();
        return;
    }

    for (int i=1 ; i<argc ; i++ ) {
        TString arcName(argv[i]);
        CreateEmptyArchive(arcName);
    }
}

int main(int argc, char *argv[]) {
    try {
        main_except_wrap(argc, argv);
    } catch (std::exception& e) {
        Cerr << "ex: " << e.what() << Endl;
        return 1;
    }
    return 0;
}
