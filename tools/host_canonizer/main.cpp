/// @author Victor Ploshikhin vvp@

#include <kernel/erfcreator/canonizers.h>

#include <library/cpp/getopt/opt2.h>

#include <util/generic/map.h>
#include <util/stream/file.h>
#include <util/stream/input.h>

class TCanonizersHolder {
private:
    typedef TMap<TString, const TCombinedHostCanonizer*> TCanonizerMap;

    TCanonizers         Canonizers;
    TCanonizerMap       CanonizersMap;

    const TCombinedHostCanonizer* CurCanonizer;

    IInputStream*       InputStream;

private:
    void FillCanonizersMap();

public:
    void Init(const TString&, const TString&, const TString&, const TString&);

    TString GetCanonizerNames() const;

    void Process() const;
};

namespace {

THostCanonizerPtr CreateMirrorCanonizer(const TString& mirrFile)
{
    if ( mirrFile.EndsWith(".trie") ) {
        return new TMirrorsMappedTrieCanonizer(mirrFile);
    } else if ( mirrFile.EndsWith(".hash") ) {
        return new TMirrorMappedHostCanonizer(mirrFile);
    }
    return new TMirrorHostCanonizer(mirrFile);
}

THostCanonizerPtr GetOwnerCanonizer(const TString& ownerFile)
{
    return new TOwnerHostCanonizer(ownerFile);
}

}
void TCanonizersHolder::Init(const TString& canonType, const TString& mirrFile, const TString& ownerFile, const TString& inFile)
{
    Canonizers.InitCanonizers(CreateMirrorCanonizer(mirrFile), GetOwnerCanonizer(ownerFile));

    FillCanonizersMap();

    TCanonizerMap::iterator it = CanonizersMap.find(canonType);
    if ( CanonizersMap.end() == it ) {
        throw yexception() << "can not find canonizer of type '" << canonType << "'";
    }
    CurCanonizer = it->second;

    if ( inFile.empty() == false ) {
        InputStream = new TUnbufferedFileInput(inFile);
    } else {
        InputStream = &Cin;
    }
}

void TCanonizersHolder::FillCanonizersMap()
{
#define CANON(NAME, FIELD) CanonizersMap[#NAME] = Canonizers.FIELD.Get();
    CANON(mirror, MirrorCanonizer)
    CANON(owner, OwnerCanonizer)
    CANON(mirror-owner, MirrorOwnerCanonizer)
    CANON(strip-www, StripWWWCanonizer)
    CANON(mirror-owner-www, MirrorOwnerWWWCanonizer)
    CANON(owner-mirror, OwnerMirrorCanonizer)
    CANON(reverse, ReverseDomainCanonizer)
#undef CANON
}

TString TCanonizersHolder::GetCanonizerNames() const
{
    TString ret;
    for ( TCanonizerMap::const_iterator it = CanonizersMap.begin(); it != CanonizersMap.end(); ++it ) {
        ret += it->first;
        ret += ", ";
    }
    return ret;
}

void TCanonizersHolder::Process() const
{
    TString url;
    while ( InputStream->ReadLine(url) ) {
        Cout << CurCanonizer->CanonizeHost(url) << Endl;
    }
}

int main(int argc, char** argv)
{
    try {
        TCanonizersHolder canHolder;

        Opt2 opt(argc, argv, "t:m:o:i:");

        const char * type       = opt.Arg('t', (TString("type: ") + canHolder.GetCanonizerNames()).data(), nullptr, true);
        const char * mirrorPath = opt.Arg('m', "path to mirrors file", "/Berkanavt/config/mirrors.res", false);
        const char * ownerPath  = opt.Arg('o', "path to owners file", "/Berkanavt/catalog/areas.lst", false);
        const char * inFile     = opt.Arg('i', "input file, in this case stdin will not be used", "", false);

        opt.AutoUsageErr("");

        canHolder.Init(type, mirrorPath, ownerPath, inFile);

        canHolder.Process();

    } catch ( const yexception& ex ) {
        Cerr << "error: " << CurrentExceptionMessage() << Endl;
    }

    return 0;
}
