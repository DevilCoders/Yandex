#pragma once

#include "xmltree.h"

#include <util/generic/ptr.h>

#include <library/cpp/numerator/numerate.h>

namespace NHtmlTree {
    /*! Builds XmlTree from HtmlChunks (elements of TStorage) with libxml
*
*   Invoke OnHtmlChunk to add new chunk (one should use it while constructing tree THtmlChunk by THtmlChunk)
*   One may also call BuildTree(const TStorage&) to construct tree from TStorage
*
*   @note When your get pointer to tree with GetTreePointer() method you must guarantee that you wouldn't change tree if you want to work with this specific object later
*/
    class TXmlDomBuilder: public INumeratorHandler {
    public:
        TXmlDomBuilder(const IParsedDocProperties& docProperites);
        ~TXmlDomBuilder() override;

        void OnMoveInput(const THtmlChunk& chunk, const TZoneEntry*, const TNumerStat& numerStat) override;

        TSimpleSharedPtr<TXmlTree> GetTree() const;

    private:
        class TImpl;
        THolder<TImpl> Impl;
    };

}
