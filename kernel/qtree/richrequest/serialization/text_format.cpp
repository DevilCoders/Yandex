#include <float.h>
#include <math.h>
#include <cstdio>
#include <stack>
#include <limits>

#include "text_format.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/stubs/strutil.h>

using namespace google::protobuf;

namespace NRichTreeProtocol {

// ===========================================================================
// Internal class for parsing an ASCII representation of a Protocol Message.
// This class makes use of the Protocol Message compiler's tokenizer found
// in //google/protobuf/io/tokenizer.h. Note that class's Parse
// method is *not* thread-safe and should only be used in a single thread at
// a time.

// Makes code slightly more readable.  The meaning of "DO(foo)" is
// "Execute foo and fail if it fails.", where failure is indicated by
// returning false. Borrowed from parser.cc (Thanks Kenton!).
#define DO(STATEMENT) if (STATEMENT) {} else return false

class TextFormat::Parser::ParserImpl {
 public:

  // Determines if repeated values for a non-repeated field are
  // permitted, e.g., the string "foo: 1 foo: 2" for a
  // required/optional field named "foo".
  enum SingularOverwritePolicy {
    ALLOW_SINGULAR_OVERWRITES = 0,   // the last value is retained
    FORBID_SINGULAR_OVERWRITES = 1,  // an error is issued
  };

  ParserImpl(const Descriptor* root_message_type,
             io::ZeroCopyInputStream* input_stream,
             io::ErrorCollector* error_collector,
             SingularOverwritePolicy singular_overwrite_policy)
    : error_collector_(error_collector),
      tokenizer_error_collector_(this),
      tokenizer_(input_stream, &tokenizer_error_collector_),
      root_message_type_(root_message_type),
      singular_overwrite_policy_(singular_overwrite_policy) {
    // For backwards-compatibility with proto1, we need to allow the 'f' suffix
    // for floats.
    tokenizer_.set_allow_f_after_float(true);

    // '#' starts a comment.
    tokenizer_.set_comment_style(io::Tokenizer::SH_COMMENT_STYLE);

    // Consume the starting token.
    tokenizer_.Next();
  }
  ~ParserImpl() = default;

  // Parses the ASCII representation specified in input and saves the
  // information into the output pointer (a Message). Returns
  // false if an error occurs (an error will also be logged to
  // GOOGLE_LOG(ERROR)).
  bool Parse(Message* output) {
    DO(Consume("{"));

    // Consume fields until we cannot do so anymore.
    while(!TryConsume("}") && !LookingAtType(io::Tokenizer::TYPE_END)) {
      DO(ConsumeField(output));
    }
    return true;
  }

  void ReportError(int line, int col, const string& message) {
    if (error_collector_ == nullptr) {
      if (line >= 0) {
        GOOGLE_LOG(ERROR) << "Error parsing text-format "
                   << root_message_type_->full_name()
                   << ": " << (line + 1) << ":"
                   << (col + 1) << ": " << message;
      } else {
        GOOGLE_LOG(ERROR) << "Error parsing text-format "
                   << root_message_type_->full_name()
                   << ": " << message;
      }
    } else {
      error_collector_->AddError(line, col, message);
    }
  }

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ParserImpl);

  // Reports an error with the given message with information indicating
  // the position (as derived from the current token).
  void ReportError(const string& message) {
    ReportError(tokenizer_.current().line, tokenizer_.current().column,
                message);
  }

  // Consumes the specified message with the given starting delimeter.
  // This method checks to see that the end delimeter at the conclusion of
  // the consumption matches the starting delimeter passed in here.
  bool ConsumeMessage(Message* message, const string& delimeter0, const string& delimeter1) {
    DO(Consume(delimeter0));

    while (!LookingAt(">") &&  !LookingAt("}")) {
      DO(ConsumeField(message));
    }

    // Confirm that we have a valid ending delimeter.
    DO(Consume(delimeter1));

    return true;
  }

  // Consumes the current field (as returned by the tokenizer) on the
  // passed in message.
  bool ConsumeField(Message* message) {
    const Reflection* reflection = message->GetReflection();
    const Descriptor* descriptor = message->GetDescriptor();

    string field_name;

    const FieldDescriptor* field = nullptr;

    if (TryConsume("[")) {
      // Extension.
      DO(ConsumeIdentifier(&field_name));
      while (TryConsume(".")) {
        string part;
        DO(ConsumeIdentifier(&part));
        field_name += ".";
        field_name += part;
      }
      DO(Consume("]"));

      field = reflection->FindKnownExtensionByName(field_name);

      if (field == nullptr) {
        ReportError("Extension \"" + field_name + "\" is not defined or "
                    "is not an extension of \"" +
                    descriptor->full_name() + "\".");
        return false;
      }
    } else {
      DO(ConsumeIdentifier(&field_name));

      field = descriptor->FindFieldByName(field_name);
      // Group names are expected to be capitalized as they appear in the
      // .proto file, which actually matches their type names, not their field
      // names.
      if (field == nullptr) {
        string lower_field_name = field_name;
        LowerString(&lower_field_name);
        field = descriptor->FindFieldByName(lower_field_name);
        // If the case-insensitive match worked but the field is NOT a group,
        if (field != nullptr && field->type() != FieldDescriptor::TYPE_GROUP) {
          field = nullptr;
        }
      }
      // Again, special-case group names as described above.
      if (field != nullptr && field->type() == FieldDescriptor::TYPE_GROUP
          && field->message_type()->name() != field_name) {
        field = nullptr;
      }

      if (field == nullptr) {
        ReportError("Message type \"" + descriptor->full_name() +
                    "\" has no field named \"" + field_name + "\".");
        return false;
      }
    }

    // Fail if the field is not repeated and it has already been specified.
    if ((singular_overwrite_policy_ == FORBID_SINGULAR_OVERWRITES) &&
        !field->is_repeated() && reflection->HasField(*message, field)) {
      ReportError("Non-repeated field \"" + field_name +
                  "\" is specified multiple times.");
      return false;
    }

    // Perform special handling for embedded message types.
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      // ':' is optional here.
      TryConsume(":");

      TryConsume("["); // repeated values

      string delimeter0 = "{";
      string delimeter1 = "}";

      if (LookingAt("<")) {
        delimeter0 = "<";
        delimeter1 = ">";
      }

      if (field->is_repeated()) {
        while (!TryConsume("]")) {
          DO(ConsumeMessage(reflection->AddMessage(message, field), delimeter0, delimeter1));
          TryConsume(",");
        }
      } else {
        DO(ConsumeMessage(reflection->MutableMessage(message, field), delimeter0, delimeter1));
      }
    } else {
      DO(Consume(":"));
      TryConsume("[");
      DO(ConsumeFieldValue(message, reflection, field));
      TryConsume("]");
    }

    TryConsume(",");

    return true;
  }

  bool ConsumeFieldValue(Message* message,
                         const Reflection* reflection,
                         const FieldDescriptor* field) {

// Define an easy to use macro for setting fields. This macro checks
// to see if the field is repeated (in which case we need to use the Add
// methods or not (in which case we need to use the Set methods).
#define SET_FIELD(CPPTYPE, VALUE)                                  \
        if (field->is_repeated()) {                                \
          reflection->Add##CPPTYPE(message, field, VALUE);         \
        } else {                                                   \
          reflection->Set##CPPTYPE(message, field, VALUE);         \
        }                                                          \

    switch(field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_INT32: {
        int64 value;
        DO(ConsumeSignedInteger(&value, kint32max));
        SET_FIELD(Int32, static_cast<int32>(value));
        break;
      }

      case FieldDescriptor::CPPTYPE_UINT32: {
        uint64 value;
        DO(ConsumeUnsignedInteger(&value, kuint32max));
        SET_FIELD(UInt32, static_cast<uint32>(value));
        break;
      }

      case FieldDescriptor::CPPTYPE_INT64: {
        int64 value;
        DO(ConsumeSignedInteger(&value, kint64max));
        SET_FIELD(Int64, value);
        break;
      }

      case FieldDescriptor::CPPTYPE_UINT64: {
        uint64 value;
        DO(ConsumeUnsignedInteger(&value, kuint64max));
        SET_FIELD(UInt64, value);
        break;
      }

      case FieldDescriptor::CPPTYPE_FLOAT: {
        double value;
        DO(ConsumeDouble(&value));
        SET_FIELD(Float, static_cast<float>(value));
        break;
      }

      case FieldDescriptor::CPPTYPE_DOUBLE: {
        double value;
        DO(ConsumeDouble(&value));
        SET_FIELD(Double, value);
        break;
      }

      case FieldDescriptor::CPPTYPE_STRING: {
        string value;
        DO(ConsumeString(&value));
        SET_FIELD(String, value);
        break;
      }

      case FieldDescriptor::CPPTYPE_BOOL: {
        string value;
        DO(ConsumeIdentifier(&value));

        if (value == "true") {
          SET_FIELD(Bool, true);
        } else if (value == "false") {
          SET_FIELD(Bool, false);
        } else {
          ReportError("Invalid value for boolean field \"" + field->name()
                      + "\". Value: \"" + value  + "\".");
          return false;
        }
        break;
      }

      case FieldDescriptor::CPPTYPE_ENUM: {
        string value;
        DO(ConsumeIdentifier(&value));

        // Find the enumeration value.
        const EnumDescriptor* enum_type = field->enum_type();
        const EnumValueDescriptor* enum_value
            = enum_type->FindValueByName(value);

        if (enum_value == nullptr) {
          ReportError("Unknown enumeration value of \"" + value  + "\" for "
                      "field \"" + field->name() + "\".");
          return false;
        }

        SET_FIELD(Enum, enum_value);
        break;
      }

      case FieldDescriptor::CPPTYPE_MESSAGE: {
        // We should never get here. Put here instead of a default
        // so that if new types are added, we get a nice compiler warning.
        GOOGLE_LOG(FATAL) << "Reached an unintended state: CPPTYPE_MESSAGE";
        break;
      }
    }
#undef SET_FIELD
    return true;
  }

  // Returns true if the current token's text is equal to that specified.
  bool LookingAt(const string& text) {
    return tokenizer_.current().text == text;
  }

  // Returns true if the current token's type is equal to that specified.
  bool LookingAtType(io::Tokenizer::TokenType token_type) {
    return tokenizer_.current().type == token_type;
  }

  // Consumes an identifier and saves its value in the identifier parameter.
  // Returns false if the token is not of type IDENTFIER.
  bool ConsumeIdentifier(string* identifier) {
    if (LookingAtType(io::Tokenizer::TYPE_STRING)) {
      *identifier = tokenizer_.current().text;
      *identifier = identifier->substr(1, identifier->size() - 2); // remove quotes
    } else if (LookingAtType(io::Tokenizer::TYPE_IDENTIFIER)) {
      *identifier = tokenizer_.current().text;
    } else {
      ReportError("Expected identifier.");
      return false;
    }

    tokenizer_.Next();
    return true;
  }

  // Consumes a string and saves its value in the text parameter.
  // Returns false if the token is not of type STRING.
  bool ConsumeString(string* text) {
    if (!LookingAtType(io::Tokenizer::TYPE_STRING)) {
      ReportError("Expected string.");
      return false;
    }

    io::Tokenizer::ParseString(tokenizer_.current().text, text);

    tokenizer_.Next();
    return true;
  }

  // Consumes a uint64 and saves its value in the value parameter.
  // Returns false if the token is not of type INTEGER.
  bool ConsumeUnsignedInteger(uint64* value, uint64 max_value) {
    if (!LookingAtType(io::Tokenizer::TYPE_INTEGER)) {
      ReportError("Expected integer.");
      return false;
    }

    if (!io::Tokenizer::ParseInteger(tokenizer_.current().text,
                                     max_value, value)) {
      ReportError("Integer out of range.");
      return false;
    }

    tokenizer_.Next();
    return true;
  }

  // Consumes an int64 and saves its value in the value parameter.
  // Note that since the tokenizer does not support negative numbers,
  // we actually may consume an additional token (for the minus sign) in this
  // method. Returns false if the token is not an integer
  // (signed or otherwise).
  bool ConsumeSignedInteger(int64* value, uint64 max_value) {
    bool negative = false;

    if (TryConsume("-")) {
      negative = true;
      // Two's complement always allows one more negative integer than
      // positive.
      ++max_value;
    }

    uint64 unsigned_value;

    DO(ConsumeUnsignedInteger(&unsigned_value, max_value));

    *value = static_cast<int64>(unsigned_value);

    if (negative) {
      *value = -*value;
    }

    return true;
  }

  // Consumes a double and saves its value in the value parameter.
  // Note that since the tokenizer does not support negative numbers,
  // we actually may consume an additional token (for the minus sign) in this
  // method. Returns false if the token is not a double
  // (signed or otherwise).
  bool ConsumeDouble(double* value) {
    bool negative = false;

    if (TryConsume("-")) {
      negative = true;
    }

    // A double can actually be an integer, according to the tokenizer.
    // Therefore, we must check both cases here.
    if (LookingAtType(io::Tokenizer::TYPE_INTEGER)) {
      // We have found an integer value for the double.
      uint64 integer_value;
      DO(ConsumeUnsignedInteger(&integer_value, kuint64max));

      *value = static_cast<double>(integer_value);
    } else if (LookingAtType(io::Tokenizer::TYPE_FLOAT)) {
      // We have found a float value for the double.
      *value = io::Tokenizer::ParseFloat(tokenizer_.current().text);

      // Mark the current token as consumed.
      tokenizer_.Next();
    } else if (LookingAtType(io::Tokenizer::TYPE_IDENTIFIER)) {
      string text = tokenizer_.current().text;
      LowerString(&text);
      if (text == "inf" || text == "infinity") {
        *value = std::numeric_limits<double>::infinity();
        tokenizer_.Next();
      } else if (text == "nan") {
        *value = std::numeric_limits<double>::quiet_NaN();
        tokenizer_.Next();
      } else {
        ReportError("Expected double.");
        return false;
      }
    } else {
      ReportError("Expected double.");
      return false;
    }

    if (negative) {
      *value = -*value;
    }

    return true;
  }

  // Consumes a token and confirms that it matches that specified in the
  // value parameter. Returns false if the token found does not match that
  // which was specified.
  bool Consume(const string& value) {
    const string& current_value = tokenizer_.current().text;

    if (current_value != value) {
      ReportError("Expected \"" + value + "\", found \"" + current_value
                  + "\".");
      return false;
    }

    tokenizer_.Next();

    return true;
  }

  // Attempts to consume the supplied value. Returns false if a the
  // token found does not match the value specified.
  bool TryConsume(const string& value) {
    if (tokenizer_.current().text == value) {
      tokenizer_.Next();
      return true;
    } else {
      return false;
    }
  }

  // An internal instance of the Tokenizer's error collector, used to
  // collect any base-level parse errors and feed them to the ParserImpl.
  class ParserErrorCollector : public io::ErrorCollector {
   public:
    explicit ParserErrorCollector(TextFormat::Parser::ParserImpl* parser) :
        parser_(parser) { }

    ~ParserErrorCollector() override { };

    void AddError(int line, int column, const string& message) override {
      parser_->ReportError(line, column, message);
    }

   private:
    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ParserErrorCollector);
    TextFormat::Parser::ParserImpl* parser_;
  };

  io::ErrorCollector* error_collector_;
  ParserErrorCollector tokenizer_error_collector_;
  io::Tokenizer tokenizer_;
  const Descriptor* root_message_type_;
  SingularOverwritePolicy singular_overwrite_policy_;
};

#undef DO

// ===========================================================================
// Internal class for writing text to the io::ZeroCopyOutputStream. Adapted
// from the Printer found in //google/protobuf/io/printer.h
class TextFormat::TextGenerator {
 public:
  explicit TextGenerator(io::ZeroCopyOutputStream* output)
    : output_(output),
      buffer_(nullptr),
      buffer_size_(0),
      at_start_of_line_(true),
      failed_(false),
      indent_("") {
  }

  ~TextGenerator() {
    // Only BackUp() if we're sure we've successfully called Next() at least
    // once.
    if (buffer_size_ > 0) {
      output_->BackUp(buffer_size_);
    }
  }

  // Indent text by two spaces.  After calling Indent(), two spaces will be
  // inserted at the beginning of each line of text.  Indent() may be called
  // multiple times to produce deeper indents.
  void Indent() {
    indent_ += "    ";
  }

  // Reduces the current indent level by two spaces, or crashes if the indent
  // level is zero.
  void Outdent() {
    if (indent_.empty()) {
      GOOGLE_LOG(DFATAL) << " Outdent() without matching Indent().";
      return;
    }

    indent_.resize(indent_.size() - 4);
  }

  // Print text to the output stream.
  void Print(const string& str) {
    Print(str.c_str());
  }

  // Print text to the output stream.
  void Print(const char* text) {
    int size = strlen(text);
    int pos = 0;  // The number of bytes we've written so far.

    for (int i = 0; i < size; i++) {
      if (text[i] == '\n') {
        // Saw newline.  If there is more text, we may need to insert an indent
        // here.  So, write what we have so far, including the '\n'.
        Write(text + pos, i - pos + 1);
        pos = i + 1;

        // Setting this true will cause the next Write() to insert an indent
        // first.
        at_start_of_line_ = true;
      }
    }

    // Write the rest.
    Write(text + pos, size - pos);
  }

  // True if any write to the underlying stream failed.  (We don't just
  // crash in this case because this is an I/O failure, not a programming
  // error.)
  bool failed() const { return failed_; }

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(TextGenerator);

  void Write(const char* data, int size) {
    if (failed_) return;
    if (size == 0) return;

    if (at_start_of_line_) {
      // Insert an indent.
      at_start_of_line_ = false;
      Write(indent_.data(), indent_.size());
      if (failed_) return;
    }

    while (size > buffer_size_) {
      // Data exceeds space in the buffer.  Copy what we can and request a
      // new buffer.
      memcpy(buffer_, data, buffer_size_);
      data += buffer_size_;
      size -= buffer_size_;
      void* void_buffer;
      failed_ = !output_->Next(&void_buffer, &buffer_size_);
      if (failed_) return;
      buffer_ = reinterpret_cast<char*>(void_buffer);
    }

    // Buffer is big enough to receive the data; copy it.
    memcpy(buffer_, data, size);
    buffer_ += size;
    buffer_size_ -= size;
  }

  io::ZeroCopyOutputStream* const output_;
  char* buffer_;
  int buffer_size_;
  bool at_start_of_line_;
  bool failed_;

  string indent_;
};

// ===========================================================================

TextFormat::Parser::Parser()
  : error_collector_(nullptr),
    allow_partial_(false) {}

TextFormat::Parser::~Parser() {}

bool TextFormat::Parser::Parse(io::ZeroCopyInputStream* input,
                               Message* output) {
  output->Clear();
  ParserImpl parser(output->GetDescriptor(), input, error_collector_,
                    ParserImpl::FORBID_SINGULAR_OVERWRITES);
  return MergeUsingImpl(input, output, &parser);
}

bool TextFormat::Parser::ParseFromString(const string& input,
                                         Message* output) {
  io::ArrayInputStream input_stream(input.data(), input.size());
  return Parse(&input_stream, output);
}

bool TextFormat::Parser::Merge(io::ZeroCopyInputStream* input,
                               Message* output) {
  ParserImpl parser(output->GetDescriptor(), input, error_collector_,
                    ParserImpl::ALLOW_SINGULAR_OVERWRITES);
  return MergeUsingImpl(input, output, &parser);
}

bool TextFormat::Parser::MergeFromString(const string& input,
                                         Message* output) {
  io::ArrayInputStream input_stream(input.data(), input.size());
  return Merge(&input_stream, output);
}

bool TextFormat::Parser::MergeUsingImpl(io::ZeroCopyInputStream* /*input*/,
                                        Message* output,
                                        ParserImpl* parser_impl) {
  if (!parser_impl->Parse(output)) return false;
  if (!allow_partial_ && !output->IsInitialized()) {
    std::vector<string> missing_fields;
    output->FindInitializationErrors(&missing_fields);
    parser_impl->ReportError(-1, 0, "Message missing required fields: " +
                                    JoinStrings(missing_fields, ", "));
    return false;
  }
  return true;
}

/* static */ bool TextFormat::Parse(io::ZeroCopyInputStream* input,
                                    Message* output) {
  return Parser().Parse(input, output);
}

/* static */ bool TextFormat::Merge(io::ZeroCopyInputStream* input,
                                    Message* output) {
  return Parser().Merge(input, output);
}

/* static */ bool TextFormat::ParseFromString(const string& input,
                                              Message* output) {
  return Parser().ParseFromString(input, output);
}

/* static */ bool TextFormat::MergeFromString(const string& input,
                                              Message* output) {
  return Parser().MergeFromString(input, output);
}

/* static */ bool TextFormat::PrintToString(const Message& message,
                                            string* output) {
  GOOGLE_DCHECK(output) << "output specified is NULL";

  output->clear();
  io::StringOutputStream output_stream(output);

  bool result = Print(message, &output_stream);

  return result;
}

/* static */ bool TextFormat::PrintUnknownFieldsToString(
    const UnknownFieldSet& unknown_fields,
    string* output) {
  GOOGLE_DCHECK(output) << "output specified is NULL";

  output->clear();
  io::StringOutputStream output_stream(output);
  return PrintUnknownFields(unknown_fields, &output_stream);
}

/* static */ bool TextFormat::Print(const Message& message,
                                    io::ZeroCopyOutputStream* output) {
  TextGenerator generator(output);

  generator.Print("{\n");
  generator.Indent();
  Print(message, generator);
  generator.Outdent();
  generator.Print("}\n");

  // Output false if the generator failed internally.
  return !generator.failed();
}

/* static */ bool TextFormat::PrintUnknownFields(
    const UnknownFieldSet& unknown_fields,
    io::ZeroCopyOutputStream* output) {
  TextGenerator generator(output);

  PrintUnknownFields(unknown_fields, generator);

  // Output false if the generator failed internally.
  return !generator.failed();
}

/* static */ void TextFormat::Print(const Message& message,
                                    TextGenerator& generator) {
  const Reflection* reflection = message.GetReflection();
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);
  for (int i = 0, mi = fields.size(); i < mi; ++i) {
    PrintField(message, reflection, fields[i], generator, i != mi - 1);
  }
  PrintUnknownFields(reflection->GetUnknownFields(message), generator);
}

/* static */ void TextFormat::PrintFieldValueToString(
    const Message& message,
    const FieldDescriptor* field,
    int index,
    string* output) {

  GOOGLE_DCHECK(output) << "output specified is NULL";

  output->clear();
  io::StringOutputStream output_stream(output);
  TextGenerator generator(&output_stream);

  PrintFieldValue(message, message.GetReflection(), field, index, generator);
}

/* static */ void TextFormat::PrintField(const Message& message,
                                         const Reflection* reflection,
                                         const FieldDescriptor* field,
                                         TextGenerator& generator,
                                         bool comma) {
  int count = 0;

  if (field->is_repeated()) {
    count = reflection->FieldSize(message, field);
  } else if (reflection->HasField(message, field)) {
    count = 1;
  }

  if (field->is_repeated()) {
    generator.Print("\"");
    generator.Print(field->name());
    generator.Print("\": [\n");
    generator.Indent();
  }

  for (int j = 0; j < count; ++j) {
    if (field->is_extension()) {
      generator.Print("[");
      // We special-case MessageSet elements for compatibility with proto1.
      if (field->containing_type()->options().message_set_wire_format()
          && field->type() == FieldDescriptor::TYPE_MESSAGE
          && field->is_optional()
          && field->extension_scope() == field->message_type()) {
        generator.Print(field->message_type()->full_name());
      } else {
        generator.Print(field->full_name());
      }
      generator.Print("]");
    } else {
      if (field->type() == FieldDescriptor::TYPE_GROUP) {
        // Groups must be serialized with their original capitalization.
        generator.Print(field->message_type()->name());
      } else {
        if (!field->is_repeated()) {
          generator.Print("\"");
          generator.Print(field->name());
          generator.Print("\"");
        }
      }
    }

    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (!field->is_repeated()) {
        generator.Print(": ");
      }
      generator.Print("{\n");
      generator.Indent();
    } else {
      if (!field->is_repeated()) {
        generator.Print(": ");
      }
    }

    // Write the field value.
    int field_index = j;
    if (!field->is_repeated()) {
      field_index = -1;
    }

    PrintFieldValue(message, reflection, field, field_index, generator);

    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      generator.Outdent();
      generator.Print("}");
    }

    if ((comma && !field->is_repeated()) || (field->is_repeated() && j < count -1)) {
      generator.Print(",");
    }
    generator.Print("\n");
  }

  if (field->is_repeated()) {
    generator.Outdent();
    generator.Print("]");
    if (comma) {
      generator.Print(",");
    }
    generator.Print("\n");
  }
}

static int EscapeStringImpl(const char* src, int src_len, char* dest,
                           int dest_len) {
  const char* src_end = src + src_len;
  int used = 0;

  for (; src < src_end; src++) {
    if (dest_len - used < 2)   // Need space for two letter escape
      return -1;

    switch (*src) {
      case '\"': dest[used++] = '\\'; dest[used++] = '\"'; break;
      case '\\': dest[used++] = '\\'; dest[used++] = '\\'; break;
      //case '/': dest[used++] = '\\'; dest[used++] = '/'; break; // this escape demands changes at protobuf/io/tokenizer.cc
      case '\b': dest[used++] = '\\'; dest[used++] = 'b';  break;
      case '\f': dest[used++] = '\\'; dest[used++] = 'f';  break;
      case '\n': dest[used++] = '\\'; dest[used++] = 'n';  break;
      case '\r': dest[used++] = '\\'; dest[used++] = 'r';  break;
      case '\t': dest[used++] = '\\'; dest[used++] = 't';  break;
      default:
        dest[used++] = *src; break;
    }
  }

  if (dest_len - used < 1)   // make sure that there is room for \0
    return -1;

  dest[used] = '\0';   // doesn't count towards return value though
  return used;
}

static string EscapeString(const string& src) {
  const size_t dest_length = src.size() * 4 + 1; // Maximum possible expansion
  std::vector<char> dest(dest_length);
  const int len = EscapeStringImpl(src.data(), src.size(),
                                  dest.data(), dest_length);
  GOOGLE_DCHECK_GE(len, 0);
  return string(dest.data(), len);
}

/* static */ void TextFormat::PrintFieldValue(
    const Message& message,
    const Reflection* reflection,
    const FieldDescriptor* field,
    int index,
    TextGenerator& generator) {
  GOOGLE_DCHECK(field->is_repeated() || (index == -1))
      << "Index must be -1 for non-repeated fields";

  switch (field->cpp_type()) {
#define OUTPUT_FIELD(CPPTYPE, METHOD, TO_STRING)                             \
      case FieldDescriptor::CPPTYPE_##CPPTYPE:                               \
        generator.Print(TO_STRING(field->is_repeated() ?                     \
          reflection->GetRepeated##METHOD(message, field, index) :           \
          reflection->Get##METHOD(message, field)));                         \
        break;                                                               \

      OUTPUT_FIELD( INT32,  Int32, SimpleItoa);
      OUTPUT_FIELD( INT64,  Int64, SimpleItoa);
      OUTPUT_FIELD(UINT32, UInt32, SimpleItoa);
      OUTPUT_FIELD(UINT64, UInt64, SimpleItoa);
      OUTPUT_FIELD( FLOAT,  Float, SimpleFtoa);
      OUTPUT_FIELD(DOUBLE, Double, SimpleDtoa);
#undef OUTPUT_FIELD

      case FieldDescriptor::CPPTYPE_STRING: {
        string scratch;
        const string& value = field->is_repeated() ?
            reflection->GetRepeatedStringReference(
              message, field, index, &scratch) :
            reflection->GetStringReference(message, field, &scratch);

        generator.Print("\"");
        generator.Print(EscapeString(value));
        generator.Print("\"");

        break;
      }

      case FieldDescriptor::CPPTYPE_BOOL:
        if (field->is_repeated()) {
          generator.Print(reflection->GetRepeatedBool(message, field, index)
                          ? "true" : "false");
        } else {
          generator.Print(reflection->GetBool(message, field)
                          ? "true" : "false");
        }
        break;

      case FieldDescriptor::CPPTYPE_ENUM:
        generator.Print(field->is_repeated() ?
          reflection->GetRepeatedEnum(message, field, index)->name() :
          reflection->GetEnum(message, field)->name());
        break;

      case FieldDescriptor::CPPTYPE_MESSAGE:
        Print(field->is_repeated() ?
                reflection->GetRepeatedMessage(message, field, index) :
                reflection->GetMessage(message, field),
              generator);
        break;
  }
}

// Prints an integer as hex with a fixed number of digits dependent on the
// integer type.
template<typename IntType>
static string PaddedHex(IntType value) {
  string result;
  result.reserve(sizeof(value) * 2);
  for (int i = sizeof(value) * 2 - 1; i >= 0; i--) {
    result.push_back(int_to_hex_digit(value >> (i*4) & 0x0F));
  }
  return result;
}

/* static */ void TextFormat::PrintUnknownFields(
    const UnknownFieldSet& unknown_fields, TextGenerator& generator) {
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);
    string field_number = SimpleItoa(field.number());


    switch (field.type())
    {
      case UnknownField::TYPE_VARINT:
      {
        generator.Print(field_number);
        generator.Print(": ");
        generator.Print(SimpleItoa(field.varint()));
        generator.Print("\n");
        break;
      }

      case UnknownField::TYPE_FIXED32:
      {
        generator.Print(field_number);
        generator.Print(": 0x");
        char buffer[kFastToBufferSize];
        generator.Print(FastHex32ToBuffer(field.fixed32(), buffer));
        generator.Print("\n");
        break;
      }

      case UnknownField::TYPE_FIXED64:
      {
        generator.Print(field_number);
        generator.Print(": 0x");
        char buffer[kFastToBufferSize];
        generator.Print(FastHex64ToBuffer(field.fixed64(), buffer));
        generator.Print("\n");
        break;
      }

      case UnknownField::TYPE_LENGTH_DELIMITED:
      {
        generator.Print(field_number);
        const string& value = field.length_delimited();
        UnknownFieldSet embedded_unknown_fields;
        if (!value.empty() && embedded_unknown_fields.ParseFromString(value)) {
          // This field is parseable as a Message.
          // So it is probably an embedded message.
          generator.Print(" {\n");
          generator.Indent();
          PrintUnknownFields(embedded_unknown_fields, generator);
          generator.Outdent();
          generator.Print("}\n");
        } else {
          // This field is not parseable as a Message.
          // So it is probably just a plain string.
          generator.Print(": \"");
          generator.Print(CEscape(value));
          generator.Print("\"\n");
        }
        break;
      }

      case UnknownField::TYPE_GROUP:
      {
        generator.Print(field_number);
        generator.Print(" {\n");
        generator.Indent();
        PrintUnknownFields(field.group(), generator);
        generator.Outdent();
        generator.Print("}\n");
        break;
      }
    }
  }
}

}  // namespace NRichTreeProtocol
