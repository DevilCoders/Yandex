#include "catfilter_wrapper.h"

#include "catfilter.h"
#include "catfiltertrie.h"

ICatFilter* GetCatFilter(const TString& filename, bool map, TMirrorsMappedTrie* mirrors) {
    return
        filename.EndsWith(".trie")
            ? (ICatFilter*)new TCatFilterTrie(filename, mirrors)
            : (ICatFilter*)new TCatFilterObj(filename, map);
}

TCatFilterObj::TCatFilterObj(const TString& filename, bool map) {
    TCatFilter* filter;
    MyFilter.Reset(filter = new TCatFilter);
    if (map)
        filter->Map(filename.data());
    else
        filter->Load(filename.data());
    Filter = filter;
}

TCatFilterObj::TCatFilterObj(const TCatFilter* filter)
    : Filter(filter)
{ }

TCatAttrsPtr TCatFilterObj::Find(const TString& url) const {
    return Filter->Find0(url.data());
}
