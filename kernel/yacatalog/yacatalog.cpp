#include "yacatalog.h"

#include <library/cpp/xml/sax/simple.h>

#include <library/cpp/deprecated/split/delim_string_iter.h>
#include <util/generic/stack.h>
#include <util/stream/file.h>
#include <util/string/split.h>

#include <algorithm>



struct TYaCatalogXmlParser : public NXml::ISimpleSaxHandler {
    void OnStartElement(const TStringBuf& name, const TAttr* attrs, size_t count) override {
        if (name == TStringBuf("cat")) {
            TCatalogCategory cat = 0;
            TCatalogCategoryDescr descr;

            for (size_t i = 0; i < count; ++i) {
                const TAttr& attr = attrs[i];

                if (attr.Name == TStringBuf("id")) {
                    if (!attr.Value.empty()) {
                        cat = FromString<TCatalogCategory>(attr.Value);
                    }
                } else if (attr.Name == TStringBuf("parent_id")) {
                    if (!attr.Value.empty()) {
                        descr.Parent = FromString<TCatalogCategory>(attr.Value);
                    }
                } else if (attr.Name == TStringBuf("name")) {
                    descr.Name = (attr.Value);
                } else if (attr.Name == TStringBuf("url_name")) {
                    descr.UrlName = (attr.Value);
                } else if (attr.Name == TStringBuf("type")) {
                    descr.Type = FromString<ui32>(attr.Value);
                } else if (attr.Name == TStringBuf("hide")) {
                    descr.Hide = FromString<bool>(attr.Value);
                }
            }

            if (cat) {
                CategoriesPtr[cat] = descr;
            }
        }
    }

    void OnEndElement(const TStringBuf& /*name*/) override {}
    void OnText(const TStringBuf& /*text*/) override {}

    inline TYaCatalogXmlParser(TCategoriesMap & categoriesPtr)
        : CategoriesPtr(categoriesPtr)
    {
    }

    TCategoriesMap & CategoriesPtr;
};


REGISTER_SAVELOAD_CLASS(0x0BF59B03, TCatalogParser);

TCatalogParser::TCatalogParser(TString yaCatalogXmlFile)
{
    TFileInput input(yaCatalogXmlFile);

    TYaCatalogXmlParser xmlParser(Categories);
    NXml::Parse(input, &xmlParser);

}

const TCatalogCategoryDescr &  TCatalogParser::GetCategoryDescr(const TCatalogCategory cat) const
{
    TCategoriesMap::const_iterator iter = Categories.find(cat);
    if (iter == Categories.end()) {
        ythrow yexception() << "No such category " << cat << ". (missing or invalid YaCatalog.xml?)";
    }
    return iter->second;
}

const TVector<TCatalogCategory> TCatalogParser::GetCategoryPath(const TCatalogCategory cat) const
{
    TVector<TCatalogCategory> path;

    TCatalogCategory current = cat;
    while (current != NULL_CATALOG_CATEGORY) {
        path.push_back(current);
        TCategoriesMap::const_iterator iter = Categories.find(current);
        if (iter == Categories.end()) break;
        current = iter->second.Parent;
    }
    return path;
}

// Port of arcadia/web/report/lib/YxWeb/Util/Category.pm to C++
TCatalogCategory TCatalogParser::SelectGeneral(const TVector<TCatalogCategory>& categories) const
{
    TVector< TVector<TCatalogCategory> > paths;
    paths.resize(categories.size());
    size_t max_path_len = 0;
    for (size_t cat = 0; cat < categories.size(); ++cat) {
        paths[cat] = GetCategoryPath(categories[cat]);
        if (paths[cat].size() > max_path_len) {
            max_path_len = paths[cat].size();
        }
    }

    // array of  (freq=>array) maps
    TVector<
            THashMap<size_t,
                      TVector<TCatalogCategory>
                     >
            > combined_path;

    combined_path.resize(max_path_len);

    TVector<size_t> max_intersections;
    max_intersections.resize(max_path_len);

    for (size_t i = 0; i < max_path_len; ++i) {
        THashMap<TCatalogCategory, size_t> element_freqs;

        for (size_t j = 0; j < paths.size(); ++j) {
            if (i < paths[j].size()) {
                element_freqs[paths[j][paths[j].size() - i - 1]] += 1;
            }
        }

        for (THashMap<TCatalogCategory, size_t>::const_iterator it = element_freqs.begin();
             it != element_freqs.end();
             ++it)
        {
            combined_path[i][it->second].push_back(it->first);

            if (it->second > max_intersections[i]) {
                max_intersections[i] = it->second;
            }
        }
    }

    THashMap<size_t, size_t> level_by_intersection;

    for (size_t i = 0; i< combined_path.size(); ++i) {
        level_by_intersection[max_intersections[i]] = i;
    }

    for (size_t intersection = 2; intersection < categories.size() + 1; ++intersection) {
        THashMap<size_t, size_t>::const_iterator it = level_by_intersection.find(intersection);
        if (it != level_by_intersection.end()) {
            size_t level = it->second;

            if (level > 0) {
                TVector<TCatalogCategory> * ids = &combined_path[level][intersection];

                if ( ( ids->size() > 1 ) && ( level > 1 ) ) {
                    ids = &combined_path[1] [max_intersections[1]];
                }

                return *std::max_element(ids->begin(), ids->end());
            }
        }
    }

    return *std::max_element(categories.begin(), categories.end());
}

TCatalogCategory TCatalogParser::SelectGeneral(const TString & categories) const
{
    TVector<TCatalogCategory> vec;
    StringSplitter(TStringBuf(categories)).Split(',').ParseInto(&vec);
    return SelectGeneral(vec);
}


