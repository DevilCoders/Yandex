#pragma once

class IOutputStream;
class IParsedDocProperties;

namespace NHtml {
    class TStorage;

    void ConvertChunksToXml(const TStorage* storage, IParsedDocProperties* docProps, IOutputStream& resultStream);

}
