// Redefenition of google::protobuf::TextFormat classes.
// Actually this has to be inheritance, but authors have made everything to prevent it in any way.

#ifndef text_format_h
#define text_format_h

#include <google/protobuf/text_format.h>

namespace NRichTreeProtocol {

typedef google::protobuf::string string;
typedef google::protobuf::Message Message;
typedef google::protobuf::UnknownFieldSet UnknownFieldSet;
typedef google::protobuf::FieldDescriptor FieldDescriptor;
typedef google::protobuf::Reflection Reflection;

// This class implements protocol buffer text format.  Printing and parsing
// protocol messages in text format is useful for debugging and human editing
// of messages.
//
// This class is really a namespace that contains only static methods.
class LIBPROTOBUF_EXPORT TextFormat {
 public:
  // Outputs a textual representation of the given message to the given
  // output stream.
  static bool Print(const Message& message, google::protobuf::io::ZeroCopyOutputStream* output);

  // Print the fields in an UnknownFieldSet.  They are printed by tag number
  // only.  Embedded messages are heuristically identified by attempting to
  // parse them.
  static bool PrintUnknownFields(const UnknownFieldSet& unknown_fields,
                                 google::protobuf::io::ZeroCopyOutputStream* output);

  // Like Print(), but outputs directly to a string.
  static bool PrintToString(const Message& message, string* output);

  // Like PrintUnknownFields(), but outputs directly to a string.
  static bool PrintUnknownFieldsToString(const UnknownFieldSet& unknown_fields,
                                         string* output);

  // Outputs a textual representation of the value of the field supplied on
  // the message supplied. For non-repeated fields, an index of -1 must
  // be supplied. Note that this method will print the default value for a
  // field if it is not set.
  static void PrintFieldValueToString(const Message& message,
                                      const FieldDescriptor* field,
                                      int index,
                                      string* output);

  // Parses a text-format protocol message from the given input stream to
  // the given message object.  This function parses the format written
  // by Print().
  static bool Parse(google::protobuf::io::ZeroCopyInputStream* input, Message* output);
  // Like Parse(), but reads directly from a string.
  static bool ParseFromString(const string& input, Message* output);

  // Like Parse(), but the data is merged into the given message, as if
  // using Message::MergeFrom().
  static bool Merge(google::protobuf::io::ZeroCopyInputStream* input, Message* output);
  // Like Merge(), but reads directly from a string.
  static bool MergeFromString(const string& input, Message* output);

  // For more control over parsing, use this class.
  class LIBPROTOBUF_EXPORT Parser {
   public:
    Parser();
    ~Parser();

    // Like TextFormat::Parse().
    bool Parse(google::protobuf::io::ZeroCopyInputStream* input, Message* output);
    // Like TextFormat::ParseFromString().
    bool ParseFromString(const string& input, Message* output);
    // Like TextFormat::Merge().
    bool Merge(google::protobuf::io::ZeroCopyInputStream* input, Message* output);
    // Like TextFormat::MergeFromString().
    bool MergeFromString(const string& input, Message* output);

    // Set where to report parse errors.  If NULL (the default), errors will
    // be printed to stderr.
    void RecordErrorsTo(google::protobuf::io::ErrorCollector* error_collector) {
      error_collector_ = error_collector;
    }

    // Normally parsing fails if, after parsing, output->IsInitialized()
    // returns false.  Call AllowPartialMessage(true) to skip this check.
    void AllowPartialMessage(bool allow) {
      allow_partial_ = allow;
    }

   private:
    // Forward declaration of an internal class used to parse text
    // representations (see text_format.cc for implementation).
    class ParserImpl;

    // Like TextFormat::Merge().  The provided implementation is used
    // to do the parsing.
    bool MergeUsingImpl(google::protobuf::io::ZeroCopyInputStream* input,
                        Message* output,
                        ParserImpl* parser_impl);

    google::protobuf::io::ErrorCollector* error_collector_;
    bool allow_partial_;
  };

 private:
  // Forward declaration of an internal class used to print the text
  // output to the OutputStream (see text_format.cc for implementation).
  class TextGenerator;

  // Internal Print method, used for writing to the OutputStream via
  // the TextGenerator class.
  static void Print(const Message& message,
                    TextGenerator& generator);

  // Print a single field.
  static void PrintField(const Message& message,
                         const Reflection* reflection,
                         const FieldDescriptor* field,
                         TextGenerator& generator,
                         bool comma = false);

  // Outputs a textual representation of the value of the field supplied on
  // the message supplied or the default value if not set.
  static void PrintFieldValue(const Message& message,
                              const Reflection* reflection,
                              const FieldDescriptor* field,
                              int index,
                              TextGenerator& generator);

  // Print the fields in an UnknownFieldSet.  They are printed by tag number
  // only.  Embedded messages are heuristically identified by attempting to
  // parse them.
  static void PrintUnknownFields(const UnknownFieldSet& unknown_fields,
                                 TextGenerator& generator);

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(TextFormat);
};

}  // namespace NRichTreeProtocol

#endif  // text_format_h
