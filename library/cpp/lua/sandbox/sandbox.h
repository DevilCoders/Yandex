#pragma once

#include <library/cpp/lua/wrapper.h>
#include <util/generic/noncopyable.h>

namespace NLua {
    class TSandbox: public TNonCopyable {
    public:
        TSandbox(bool emptyEnv = false, size_t sizeLimit = 10 * 1024 * 1024);

        // sandboxed script - can access only whitelisted functions
        // cannot be used after death of TSandbox object
        // NonCopyable because object address used as a key (sandbox[this] = compiled_script)
        class TScript: public TNonCopyable {
        public:
            enum EScriptType {
                ST_TEXT,
                ST_BYTECODE,
                ST_DETECT
            };

            // put user script into the sandbox
            TScript(TSandbox& sandbox,
                    const TStringBuf& script,
                    bool emptyEnv = false,
                    EScriptType type = ST_TEXT);
            ~TScript();

            // push sandboxed function onto the stack
            // use State.push_* to set arguments after pushing
            // and State.call to call this function
            void Push();

            void InitEnv();
            void UpdateEnv(const TStringBuf& name, const TStringBuf& object = "");

            // sandbox and lua state
            TSandbox& Sandbox;
            TLuaStateHolder& State;

        private:
            void Load(const TStringBuf& script, bool emptyEnv, EScriptType type);
            void SaveEnv();
            void SaveFunction();
        };

        // initialize white list with safe standart functions
        void InitEnv();

        // add global object to the white list (env[name] = object)
        // if object is empty string then remove name from env
        void UpdateEnv(const TStringBuf& name, const TStringBuf& object = "");

    public:
        TLuaStateHolder State;
    };

}
