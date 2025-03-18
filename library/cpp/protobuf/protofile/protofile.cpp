#include "protofile.h"

#include <google/protobuf/text_format.h>

#include <library/cpp/logger/global/global.h>

#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <errno.h>

namespace NFastTier {
        bool TGeneralBinaryProtoReader::ReadDump(void* ptr, size_t size) {
            size_t bytesLoaded = Stream->Load(ptr, size);
            Position += bytesLoaded;
            return bytesLoaded == size;
        }

        void TGeneralBinaryProtoReader::OpenWithMaxSize(IInputStream* stream, size_t maxItemSize) {
            Stream = stream;
            MaxItemSize = maxItemSize;
            Position = 0;
        }

        void TGeneralBinaryProtoReader::Open(const TString& fileName) {
            StreamHolder.Reset(new TIFStream(fileName));
            IGeneralProtoReader::Open(StreamHolder.Get());
        }

        void TGeneralBinaryProtoReader::ForwardSeek(size_t newPosition) {
            Y_ENSURE(Stream, "Stream must be open. ");
            if (Position != newPosition) {
                Y_ENSURE(newPosition >= Position,
                         "Current position in the stream (" << Position << ") is past the requested one (" << newPosition << ") in ForwardSeek. ");
                Stream->Skip(newPosition - Position);
                Position = newPosition;
            }
        }

        bool TGeneralBinaryProtoReader::GetNextMessage(::google::protobuf::Message& record) {
            Y_ENSURE(Stream, "Stream must be open. ");
            ui32 recordSize = 0;
            if (!ReadDump(&recordSize, sizeof(ui32))) {
                return false;
            }
            Y_ENSURE(MaxItemSize > recordSize, "protobuf length is too high: " << recordSize);
            Buffer.Resize(recordSize);
            Y_ENSURE(ReadDump(Buffer.data(), recordSize), "Corrupted record in protofile. ");
            record.Clear();
            Y_ENSURE(record.ParseFromArray(Buffer.data(), recordSize),
                     "Corrupted record in protofile. ");
            return true;
        }

        void TGeneralBinaryProtoWriter::WriteDump(void* ptr, size_t size) {
            Stream->Write(ptr, size);
            Position += size;
        }

        void TGeneralBinaryProtoWriter::Open(IOutputStream* stream) {
            Stream = stream;
            Position = 0;
        }

        void TGeneralBinaryProtoWriter::Open(const TString& fileName) {
            StreamHolder.Reset(new TOFStream(fileName));
            Stream = StreamHolder.Get();
            Position = 0;
        }

        void TGeneralBinaryProtoWriter::WriteMessage(const ::google::protobuf::Message& record) {
            Y_ENSURE(Stream, "Stream must be open. ");
            ui32 recordSize = record.ByteSize();
            WriteDump(&recordSize, sizeof(ui32));
            Y_ENSURE(record.SerializeToArcadiaStream(Stream), "Failed to serialize record");
            Position += recordSize;
        }

        void TGeneralBinaryProtoWriter::Finish() {
            Y_ENSURE(Stream, "Stream must be open. ");
            Stream->Finish();
            StreamHolder.Destroy();
        }

        void TGeneralTextProtoReader::OpenWithMaxSize(IInputStream* stream, size_t) {
            TextInput = stream;
        }

        void TGeneralTextProtoReader::Open(const TString& file) {
            TextInputHolder.Reset(new TIFStream(file));
            TextInput = TextInputHolder.Get();
        }

        bool TGeneralTextProtoReader::GetNextMessage(::google::protobuf::Message& record) {
            Y_ENSURE(TextInput, "Stream must be open. ");
            TString tmpString;
            TString line;
            bool lastReadSuccess;
            while ((lastReadSuccess = TextInput->ReadLine(line))) {
                if (line.empty()) {
                    break;
                }
                tmpString += line;
                tmpString.push_back('\n');
            }
            if (!lastReadSuccess && tmpString.empty()) {
                return false;
            }
            google::protobuf::TextFormat::ParseFromString(tmpString, &record);
            return true;
        }

        void TGeneralTextProtoWriter::Open(IOutputStream* stream) {
            TextOutput = stream;
        }

        void TGeneralTextProtoWriter::Open(const TString& file) {
            TextOutputHolder.Reset(new TOFStream(file));
            TextOutput = TextOutputHolder.Get();
        }

        void TGeneralTextProtoWriter::WriteMessage(const ::google::protobuf::Message& record) {
            Y_ENSURE(TextOutput, "Stream must be open. ");
            TString tmpString;
            google::protobuf::TextFormat::PrintToString(record, &tmpString);
            *TextOutput << tmpString << TStringBuf("\n"); // Extra Endline is the record separator, don't use Endl to avoid flushing.
        }

        void TGeneralTextProtoWriter::Finish() {
            Y_ENSURE(TextOutput, "Stream must be open. ");
            TextOutput->Flush();
            TextOutputHolder.Destroy();
        }

        TGeneralBinaryProtoReader::~TGeneralBinaryProtoReader() = default;
        TGeneralBinaryProtoWriter::~TGeneralBinaryProtoWriter() = default;
        TGeneralTextProtoReader::~TGeneralTextProtoReader() = default;
        TGeneralTextProtoWriter::~TGeneralTextProtoWriter() = default;
}

namespace NDetail {
    TProtoFileGuardBase::TProtoFileGuardBase(::google::protobuf::Message& proto, const TFsPath& path, bool forceOpen, bool allowUnknownFields)
        : Proto(proto)
        , Path(path)
        , Deleted(false)
        , Changed(false)
    {
        if (!path.Exists()) {
            return;
        }

        TFileInput fi(path);
        OldContent = fi.ReadAll();
        ::google::protobuf::TextFormat::Parser parser;
        parser.AllowUnknownField(allowUnknownFields);
        if (forceOpen) {
            if (!parser.ParseFromString(OldContent, &Proto)) {
                path.ForceDelete();
                Deleted = true;
            }
        } else {
            VERIFY_WITH_LOG(parser.ParseFromString(OldContent, &Proto), "Corrupted %s", Path.GetPath().data());
        }
    }

    void TProtoFileGuardBase::Flush() {
        if (!Changed)  // We should probably not exit from here if |Deleted| == true. But such a change affects current observable behavior
            return;
        try {
            TString out;
            VERIFY_WITH_LOG(::google::protobuf::TextFormat::PrintToString(Proto, &out), "Error while serializing %s", Path.GetPath().data());
            if (!Deleted && out == OldContent) {
                return;
            }
            TFsPath tmpPath = Path.Parent() / ("~" + Path.GetName());
            {
                TUnbufferedFileOutput fo(tmpPath);
                fo.Write(out.data(), out.size());
                fo.Finish();
            }
            tmpPath.ForceRenameTo(Path);
        } catch (...) {
            FAIL_LOG("cannot save %s: %s, errno = %i", Path.GetPath().data(), CurrentExceptionMessage().data(), errno);
        }
    }

    TProtoFileGuardBase::~TProtoFileGuardBase() {
        Flush();
    }
}
