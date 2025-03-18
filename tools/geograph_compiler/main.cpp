#include <kernel/gazetteer/gazetteer.h>
#include <kernel/geograph/geograph.h>

#include <util/stream/file.h>

int main(int argc, const char** argv) {
    if (argc != 3) {
        Cerr << "usage: " << argv[0] << " <gazetteer input> <geograph output>" << Endl;
        return 1;
    }
    NGzt::TGazetteer gzt(TBlob::FromFile(argv[1]));
    NGeoGraph::TGeoGraph graph(&gzt);
    TFileOutput out(argv[2]);
    graph.Save(out);
    return 0;
}
