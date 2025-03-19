#pragma once

#include <type_traits>

#include <kernel/doom/progress/null_progress_callback.h>

namespace NDoom {

namespace NPrivate {

// TODO: should probably merge these classes. Think of a good iface.

enum EReaderClass {
    KeyInvReader,
    KeyInvLayeredReader,
    DocInvReader
};

template<class Reader, class = decltype(std::declval<Reader>().NextLayer())>
std::true_type HasNextLayerImpl(const Reader&);

std::false_type HasNextLayerImpl(...);

template<class Reader>
using HasNextLayer = decltype(HasNextLayerImpl(std::declval<Reader>()));

template<class Reader, class = decltype(std::declval<Reader>().ReadKey(std::declval<typename Reader::TKeyRef*>())), class = std::enable_if_t<!HasNextLayer<Reader>::value>>
constexpr std::integral_constant<EReaderClass, KeyInvReader> ReaderClass(Reader*) { return {}; }

template<class Reader, class = decltype(std::declval<Reader>().ReadKey(std::declval<typename Reader::TKeyRef*>())), class = std::enable_if_t<HasNextLayer<Reader>::value>>
constexpr std::integral_constant<EReaderClass, KeyInvLayeredReader> ReaderClass(Reader*) { return {}; }

template<class Reader, class = decltype(std::declval<Reader>().ReadDoc(std::declval<ui32*>()))>
constexpr std::integral_constant<EReaderClass, DocInvReader> ReaderClass(Reader*) { return{}; }

template<class Reader, class Writer, class ProgressCallback>
void DoTransferIndex(Reader* reader, Writer* writer, ProgressCallback&& callback, std::integral_constant<EReaderClass, KeyInvReader>) {
    using TKeyRef = typename Reader::TKeyRef;
    using THit = typename Reader::THit;

    TKeyRef key;
    while (reader->ReadKey(&key)) {
        callback.Step(reader->Progress());

        THit hit;
        while (reader->ReadHit(&hit))
            writer->WriteHit(hit);
        writer->WriteKey(key);
    }
}

template<class Reader, class Writer, class ProgressCallback>
void DoTransferIndex(Reader* reader, Writer* writer, ProgressCallback&& callback, std::integral_constant<EReaderClass, KeyInvLayeredReader>) {
    using TKeyRef = typename Reader::TKeyRef;
    using THit = typename Reader::THit;

    TKeyRef key;
    while (reader->ReadKey(&key)) {
        callback.Step(reader->Progress());

        while (reader->NextLayer()) {
            THit hit;
            while (reader->ReadHit(&hit)) {
                writer->WriteHit(hit);
            }

            writer->WriteLayer();
        }
        writer->WriteKey(key);
    }
}

template<class Reader, class Writer, class ProgressCallback>
void DoTransferIndex(Reader* reader, Writer* writer, ProgressCallback&& callback, std::integral_constant<EReaderClass, DocInvReader>) {
    using THit = typename Reader::THit;

    ui32 doc;
    while (reader->ReadDoc(&doc)) {
        callback.Step(reader->Progress());

        THit hit;
        while (reader->ReadHit(&hit))
            writer->WriteHit(hit);
        writer->WriteDoc(doc);
    }
}

} // namespace NPrivate


template<class Reader, class Writer, class ProgressCallback>
void TransferIndex(Reader* reader, Writer* writer, ProgressCallback&& callback) {
    NPrivate::DoTransferIndex(reader, writer, std::forward<ProgressCallback>(callback), NPrivate::ReaderClass(reader));
}

template<class Reader, class Writer>
void TransferIndex(Reader* reader, Writer* writer) {
    TransferIndex(reader, writer, TNullProgressCallback());
}

template<class TReader, class = decltype(std::declval<TReader>().NextLayer())>
bool NextLayer(TReader& reader) {
    return reader.NextLayer();
}

template<class TReader>
bool NextLayer(const TReader&) {
    static_assert(!NPrivate::HasNextLayer<TReader>::value, "This overload should resolve for readers without NextLayer");
    return false;
}

} // namespace NDoom
