
/*
* simple js inteperter reads input file line by line and evals it at the end
*/

#include <contrib/libs/js-v8/include/v8.h>

#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/generic/string.h>

static const char* ToCString(const v8::String::Utf8Value& value) {
    return *value ? *value : "<string conversion failed>";
}

static TString ReadFile(const char* name) {
    TUnbufferedFileInput ifs(name);
    TStringStream ss;

    TransferData(&ifs, &ss);
    return ss.Str();
}

static v8::Handle<v8::String> ReadLine() {
    TString res = Cin.ReadLine();
    v8::Handle<v8::String> result = v8::String::New(res.data(), res.size());

    return result;
}

static v8::Handle<v8::Value> ReadLine(const v8::Arguments& args) {
    if (args.Length() > 0) {
        return v8::ThrowException(v8::String::New("Unexpected arguments"));
    }
    return ReadLine();
}

static v8::Handle<v8::Value> Print(const v8::Arguments& args) {
    bool first = true;
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope handle_scope;
        if (first) {
            first = false;
        } else {
            Cout << " ";
        }
        v8::String::Utf8Value str(args[i]);
        const char* cstr = ToCString(str);
        Cout << cstr;
    }
    Cout << Endl;
    return v8::Undefined();
}

static void ReportException(v8::TryCatch* try_catch) {
    v8::HandleScope handle_scope;
    v8::String::Utf8Value exception(try_catch->Exception());
    const char* exception_string = ToCString(exception);
    v8::Handle<v8::Message> message = try_catch->Message();
    if (message.IsEmpty()) {
        Cerr << exception_string << Endl;
    } else {
        v8::String::Utf8Value filename(message->GetScriptResourceName());
        Cerr << ToCString(filename) << ":" << message->GetLineNumber() << ": " << exception_string << Endl;

        v8::String::Utf8Value sourceline(message->GetSourceLine());
        Cerr << ToCString(sourceline) << Endl;

        int start = message->GetStartColumn();
        for (int i = 0; i < start; i++) {
            Cerr << " ";
        }
        int end = message->GetEndColumn();
        for (int i = start; i < end; i++) {
            Cerr << "^";
        }
        Cerr << Endl;
    }
}

static int RunMain(int argc, char* argv[]) {
    v8::V8::SetFlagsFromCommandLine(&argc, argv, true);
    v8::HandleScope handle_scope;

    v8::Handle<v8::String> script_source(nullptr);
    v8::Handle<v8::String> script_name(nullptr);

    TString bssource = "";
    TString bsname = "";

    bool sd = false;

    for (int i = 1; i < argc; i++) {
        const char* str = argv[i];

        if (strcmp(str, "-e") == 0 && i + 1 < argc) {
            bssource.append(TString(sd ? "\n" : "") + TString(argv[++i]));
            bsname.append(TString(sd ? ", " : "") + TString("command line"));
            sd = true;
        } else {
            bssource.append(TString(sd ? "\n" : "") + ReadFile(str));
            bsname.append(TString(sd ? ", " : "") + TString(str));
            sd = true;
        }
    }

    if (!sd) {
        Cerr << "Can't execute empty script" << Endl;
        return 1;
    }
    script_source = v8::String::New(bssource.data(), bssource.size());
    script_name = v8::String::New(bsname.data(), bsname.size());

    v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();

    global->Set(v8::String::New("print"), v8::FunctionTemplate::New(Print));
    global->Set(v8::String::New("read_line"), v8::FunctionTemplate::New(ReadLine));

    v8::Handle<v8::Context> context = v8::Context::New(nullptr, global);
    v8::Context::Scope context_scope(context);

    v8::Handle<v8::Script> script;
    {
        v8::TryCatch try_catch;
        script = v8::Script::Compile(script_source, script_name);
        if (script.IsEmpty()) {
            ReportException(&try_catch);
            return 1;
        }
        script->Run();
        if (try_catch.HasCaught()) {
            ReportException(&try_catch);
            return 1;
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    int result = RunMain(argc, argv);
    v8::V8::Dispose();
    return result;
}
