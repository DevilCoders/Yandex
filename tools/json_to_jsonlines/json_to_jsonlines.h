#pragma once

#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/stream/output.h>
#include <util/stream/input.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/system/fs.h>


namespace NJsonToJsonLines {
    enum EElemType {
        ROOT,
        DICT,
        LIST,
        NUMBER,
        STRING,
        CONSTANT
    };

    enum EConvState {
        ROOT_GENERIC,

        LIST_GENERIC,

        DICT_GENERIC,
        DICT_VALUE,
        DICT_AFTER_VALUE,

        STRING_GENERIC,
        STRING_BACKSLASH,

        NUMBER_GENERIC,

        CONSTANT_GENERIC
    };

    struct StateInfo {
        EElemType elemType;
        EConvState state;

        TString Str() const {
            TStringStream info;
            info << "SI[" << int(elemType) << ", " << int(state) << "]";
            return info.Str();
        }
    };

    bool IsJsonNumber(char ch) {
        return (isdigit(ch) or
                ch == '-' or ch == '.'
                or ch == '+' or ch == '-' or ch == 'e' or ch == 'E');
    }

    bool IsJsonConstant(char ch) {
        return (ch == 'f' or ch == 'n' or ch == 't');
    }

    bool IsJsonConstantContinuation(char ch) {
        return (
                ch == 'a' or
                ch == 'e' or
                ch == 'l' or
                ch == 'r' or
                ch == 's' or
                ch == 'u'
        );
    }

    class TJsonToJsonLinesStreamParser {
    private:
        THolder<TUnbufferedFileInput> FileInput;
        THolder<TUnbufferedFileOutput> FileOutput;

        TBufferedInput CreateInput(const TString& inputFile) {
            if (inputFile == "-") {
                return TBufferedInput(&Cin);
            }

            FileInput.Reset(new TUnbufferedFileInput(inputFile));

            return TBufferedInput(FileInput.Get());
        }

        TAdaptiveBufferedOutput CreateOutput(const TString& outputFile, const TString& tempOutputFile) {
            if (outputFile == "-") {
                return TAdaptiveBufferedOutput(&Cout);
            }

            FileOutput.Reset(new TUnbufferedFileOutput(tempOutputFile));

            return TAdaptiveBufferedOutput(FileOutput.Get());
        }

    public:
        ~TJsonToJsonLinesStreamParser() = default;

        TJsonToJsonLinesStreamParser(const TString& inputFile, const TString& outputFile, bool doConvertToJsonLines, bool isReverse = false):
                InputFile(inputFile),
                OutputFile(outputFile),
                TempOutputFile(outputFile + ".tmp"),
                DoConvertToJsonLines(doConvertToJsonLines),
                IsReverse(isReverse) {

            if (not IsReverse) {
                StateStack.reserve(10);

                StateInfo rootState{EElemType::ROOT, EConvState::ROOT_GENERIC};
                StateStack.push_back(rootState);
            }
        }

        void RunConversion() {
            TBufferedInput in = CreateInput(InputFile);
            TAdaptiveBufferedOutput out = CreateOutput(OutputFile, TempOutputFile);

            TString line;

            if (IsReverse) {
                out << "[\n";
            }

            while (in.ReadLine(line)) {
                ProcessOneLine(line, out);
                LineIndex++;
            }

            if (IsReverse) {
                out << "]";
            } else {
                CheckState();
            }

            out.Flush();

            if (OutputFile != "-") {
                NFs::Rename(TempOutputFile, OutputFile);
            }
        }

        void CheckState() {
            if (Depth() != 1) {
                ParseError("Unterminated JSON (wrong state at the end)");
            }
        }

    private:
        void ParseError(const TString& message) const {
            ythrow yexception() << "Parse error at " << LineIndex << ":" << CharIndex << ", char '" << int(CurrentChar)
                                << "', state: "
                                << CurrentStateInfo().Str() << ", details: " << message;

        }

        void WrongState() {
            TStringStream msg;
            msg << "Wrong state: " << CurrentStateInfo().Str();
            ParseError(msg.Str());
        }

        inline size_t Depth() const {
            return StateStack.size();
        }

        inline const StateInfo& CurrentStateInfo() const {
            if (StateStack.empty()) {
                ParseError("States underflow");
            }
            return StateStack.back();
        }

        inline EElemType CurrentElemType() const {
            return CurrentStateInfo().elemType;
        }

        inline EConvState CurrentState() const {
            return CurrentStateInfo().state;
        }

        inline void WrapJsonLine(TAdaptiveBufferedOutput& buffer) {
            if (DoConvertToJsonLines) {
                if (Depth() == 2) {
                    buffer << '\n';
                }
            }
        }

        inline void NextElem(TAdaptiveBufferedOutput& buffer) {
            if (DoConvertToJsonLines) {
                if (Depth() == 2) {
                    return;
                }
            }
            buffer << CurrentChar;
        }

        inline void SetState(EConvState state) {
            if (StateStack.empty()) {
                ParseError("State underflow");
            }
            StateStack.back().state = state;
        }

        inline void EnterElem(const StateInfo& stateInfo) {
            StateStack.push_back(stateInfo);
        }

        inline StateInfo LeaveElem() {
            if (StateStack.empty()) {
                ParseError("State underflow");
            }
            StateInfo stateInfo = StateStack.back();
            StateStack.pop_back();
            return stateInfo;
        }

        inline void CloseElem(TAdaptiveBufferedOutput& buffer) {
            LeaveElem();

            bool doAddChar = true;
            if (DoConvertToJsonLines) {

                if ((Depth() == 2) and (CurrentChar == ',')) {
                    doAddChar = false;
                } else if (Depth() == 1) {
                    doAddChar = false;
                }
            }

            if (doAddChar) {
                buffer << CurrentChar;
            }

            WrapJsonLine(buffer);
        }

        void ProcessRoot(TAdaptiveBufferedOutput& buffer) {
            if (CurrentChar == '[') {
                if (not DoConvertToJsonLines) {
                    buffer << CurrentChar;
                }
                EnterElem({LIST, LIST_GENERIC});
            } else if (CurrentChar == '{') {
                if (DoConvertToJsonLines) {
                    ParseError("Expected list at root");
                }
                buffer << CurrentChar;
                EnterElem({DICT, DICT_GENERIC});
            } else if (IsJsonNumber(CurrentChar)) {
                if (DoConvertToJsonLines) {
                    ParseError("Expected list at root");
                }
                buffer << CurrentChar;
                EnterElem({NUMBER, NUMBER_GENERIC});
            } else if (CurrentChar == '"') {
                if (DoConvertToJsonLines) {
                    ParseError("Expected list at root");
                }
                buffer << CurrentChar;
                EnterElem({STRING, STRING_GENERIC});
            } else if (IsJsonConstant(CurrentChar)) {
                if (DoConvertToJsonLines) {
                    ParseError("Expected list at root");
                }
                buffer << CurrentChar;
                EnterElem({CONSTANT, CONSTANT_GENERIC});
            } else if (isspace(CurrentChar)) {

            } else {
                ParseError("Unexpected char at root list");
            }

        }

        void ProcessList(TAdaptiveBufferedOutput& buffer) {
            if (CurrentChar == '[') {
                buffer << CurrentChar;
                EnterElem({LIST, LIST_GENERIC});
            } else if (CurrentChar == '{') {
                buffer << CurrentChar;
                EnterElem({DICT, DICT_GENERIC});
            } else if (CurrentChar == '"') {
                buffer << CurrentChar;
                EnterElem({STRING, STRING_GENERIC});
            } else if (IsJsonNumber(CurrentChar)) {
                buffer << CurrentChar;
                EnterElem({NUMBER, NUMBER_GENERIC});
            } else if (IsJsonConstant(CurrentChar)) {
                buffer << CurrentChar;
                EnterElem({CONSTANT, CONSTANT_GENERIC});
            } else if (CurrentChar == ',') {
                NextElem(buffer);
            } else if (CurrentChar == ']') {
                CloseElem(buffer);
            } else if (isspace(CurrentChar)) {
                // pass
            } else {
                ParseError("Unexpected char at list");
            }
        }

        void ProcessDict(TAdaptiveBufferedOutput& buffer) {
            EConvState currentState = CurrentState();
            if (currentState == DICT_GENERIC) {
                if (CurrentChar == '"') {
                    buffer << CurrentChar;
                    EnterElem({STRING, STRING_GENERIC});
                } else if (CurrentChar == '}') {
                    CloseElem(buffer);
                } else if (CurrentChar == ':') {
                    buffer << CurrentChar;
                    SetState(DICT_VALUE);
                } else if (isspace(CurrentChar)) {
                    // pass
                } else {
                    ParseError("Unexpected char on dict key");
                }
            } else if (currentState == DICT_VALUE) {
                if (IsJsonNumber(CurrentChar)) {
                    buffer << CurrentChar;
                    SetState(DICT_AFTER_VALUE);
                    EnterElem({NUMBER, NUMBER_GENERIC});
                } else if (IsJsonConstant(CurrentChar)) {
                    buffer << CurrentChar;
                    SetState(DICT_AFTER_VALUE);
                    EnterElem({CONSTANT, CONSTANT_GENERIC});
                } else if (CurrentChar == '"') {
                    buffer << CurrentChar;
                    SetState(DICT_AFTER_VALUE);
                    EnterElem({STRING, STRING_GENERIC});
                } else if (CurrentChar == '[') {
                    buffer << CurrentChar;
                    SetState(DICT_AFTER_VALUE);
                    EnterElem({LIST, LIST_GENERIC});
                } else if (CurrentChar == '{') {
                    buffer << CurrentChar;
                    SetState(DICT_AFTER_VALUE);
                    EnterElem({DICT, DICT_GENERIC});
                } else if (isspace(CurrentChar)) {
                    // pass
                } else {
                    ParseError("Unexpected char on dict value");
                }
            } else if (currentState == DICT_AFTER_VALUE) {
                if (CurrentChar == '}') {
                    CloseElem(buffer);
                } else if (CurrentChar == ',') {
                    buffer << CurrentChar;
                    SetState(DICT_GENERIC);
                } else if (CurrentChar == '"') {
                    buffer << CurrentChar;
                    SetState(DICT_GENERIC);
                    EnterElem({STRING, STRING_GENERIC});
                } else if (isspace(CurrentChar)) {
                    //
                } else {
                    ParseError("Unexpected char after dict value");
                }
            } else {
                WrongState();
            }
        }

        void ProcessString(TAdaptiveBufferedOutput& buffer) {
            EConvState currentState = CurrentState();

            if (currentState == STRING_GENERIC) {
                if (CurrentChar == '"') {
                    CloseElem(buffer);
                } else if (CurrentChar == '\\') {
                    buffer << CurrentChar;
                    SetState(STRING_BACKSLASH);
                } else {
                    buffer << CurrentChar;
                }
            } else if (currentState == STRING_BACKSLASH) {
                if (CurrentChar == '"') {
                    buffer << CurrentChar;
                    SetState(STRING_GENERIC);
                } else if (CurrentChar == '\\') {
                    buffer << CurrentChar;
                    SetState(STRING_GENERIC);
                } else if (CurrentChar == 'u') {
                    buffer << CurrentChar;
                    SetState(STRING_GENERIC);
                } else if (CurrentChar == 'n') {
                    buffer << CurrentChar;
                    SetState(STRING_GENERIC);
                } else if (CurrentChar == 't') {
                    buffer << CurrentChar;
                    SetState(STRING_GENERIC);
                } else if (CurrentChar == 'r') {
                    buffer << CurrentChar;
                    SetState(STRING_GENERIC);
                } else if (CurrentChar == 'v') {
                    buffer << CurrentChar;
                    SetState(STRING_GENERIC);
                } else if (CurrentChar == 'a') {
                    buffer << CurrentChar;
                    SetState(STRING_GENERIC);
                } else if (CurrentChar == 'b') {
                    buffer << CurrentChar;
                    SetState(STRING_GENERIC);
                } else if (CurrentChar == 'f') {
                    buffer << CurrentChar;
                    SetState(STRING_GENERIC);
                } else {
                    ParseError("Expected correct escape sequence");
                }
            } else {
                WrongState();
            }
        }

        void ProcessNumber(TAdaptiveBufferedOutput& buffer) {
            if (IsJsonNumber(CurrentChar)) {
                buffer << CurrentChar;
            } else if (CurrentChar == ']') {
                LeaveElem();
                CloseElem(buffer);
            } else if (CurrentChar == '}') {
                LeaveElem();
                CloseElem(buffer);
            } else if (CurrentChar == ',') {
                CloseElem(buffer);
            } else if (isspace(CurrentChar)) {
                LeaveElem();
            } else {
                ParseError("Unexpected char on number");
            }
        }

        void ProcessConstant(TAdaptiveBufferedOutput& buffer) {
            // n_ull t_rue f_alse
            if (IsJsonConstantContinuation(CurrentChar)) {
                buffer << CurrentChar;
            } else if (CurrentChar == ']') {
                LeaveElem();
                CloseElem(buffer);
            } else if (CurrentChar == '}') {
                LeaveElem();
                CloseElem(buffer);
            } else if (CurrentChar == ',') {
                CloseElem(buffer);
            } else if (isspace(CurrentChar)) {
                LeaveElem();
            } else {
                ParseError("Unexpected char on constant");
            }
        }

        void ProcessOneCharacter(TAdaptiveBufferedOutput& buffer) {
            EElemType elemType = CurrentElemType();

            if (elemType == EElemType::ROOT) {
                ProcessRoot(buffer);
            } else if (elemType == EElemType::LIST) {
                ProcessList(buffer);
            } else if (elemType == EElemType::DICT) {
                ProcessDict(buffer);
            } else if (elemType == EElemType::STRING) {
                ProcessString(buffer);
            } else if (elemType == EElemType::NUMBER) {
                ProcessNumber(buffer);
            } else if (elemType == EElemType::CONSTANT) {
                ProcessConstant(buffer);
            } else {
                ParseError("Wrong element type");
            }
        }

        void ProcessOneLine(const TString& line, TAdaptiveBufferedOutput& buffer) {

            if (IsReverse) {
                if (line.empty()) {
                    Cerr << "Warning: skipping empty line at index " << LineIndex << Endl;
                } else {
                    if (LineIndex > 0) {
                        buffer << ",";
                    }
                    buffer << line;
                    buffer << "\n";
                }
            } else {
                CharIndex = 0;
                for (const char ch: line) {
                    CurrentChar = ch;
                    ProcessOneCharacter(buffer);
                    CharIndex++;
                }
            }
        }

        TString InputFile;
        TString OutputFile;
        TString TempOutputFile;
        bool DoConvertToJsonLines;
        bool IsReverse;

        size_t LineIndex = 0;
        size_t CharIndex = 0;
        char CurrentChar = 0;

        TVector<StateInfo> StateStack;
    };

} /* ns NJsonToJsonLines */
