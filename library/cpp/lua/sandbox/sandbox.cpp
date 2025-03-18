#include "sandbox.h"
#include <library/cpp/resource/resource.h>

#include <util/generic/singleton.h>
#include <util/stream/buffer.h>

namespace NLua {
    struct TCompiledSandboxScript {
        TBuffer Bytecode;
        TCompiledSandboxScript() {
            Compile(NResource::Find("lua_sandbox"), Bytecode);
        }
    };

    TSandbox::TSandbox(bool emptyEnv, size_t sizeLimit)
        : State(sizeLimit)
    {
        State.BootStrap();

        // load aux sandbox functions
        TBufferInput input(Default<TCompiledSandboxScript>().Bytecode);
        State.Load(&input, "main");
        State.call(0, 0);

        // create empty table for storing user scripts
        lua_newtable(State);
        lua_setglobal(State, "sandbox");

        // create empty table to save env for each sandboxed script
        lua_newtable(State);
        lua_setglobal(State, "envs");

        // create empty table for white list
        lua_newtable(State);
        lua_setglobal(State, "env");

        if (!emptyEnv) {
            InitEnv();
        }
    }

    void TSandbox::InitEnv() {
        State.push_global("init_env");
        State.push_global("env");
        State.call(1, 0);
    }

    void TSandbox::UpdateEnv(const TStringBuf& name, const TStringBuf& object) {
        State.push_global("env");
        State.push_string(name.data());
        if (object.empty()) {
            State.push_nil();
        } else {
            State.push_global(object.data());
        }
        lua_settable(State, -3); // env[name] = object
        State.pop(1);            // remove env from stack
    }

    TSandbox::TScript::TScript(TSandbox& sandbox,
                               const TStringBuf& script,
                               bool emptyEnv,
                               EScriptType type)
        : Sandbox(sandbox)
        , State(sandbox.State)
    {
        Load(script, emptyEnv, type);
    }

    void TSandbox::TScript::Load(const TStringBuf& script, bool emptyEnv, EScriptType type) {
        // func, err = load(script, "user script", type, env)
        State.push_global("load");
        State.push_string(script);
        State.push_string("user script");

        switch (type) {
            case ST_TEXT:
                State.push_string("t");
                break;
            case ST_BYTECODE:
                State.push_string("b");
                break;
            default:
                State.push_string("bt");
        }

        if (emptyEnv) {
            lua_newtable(State);
        } else {
            State.push_global("env");
        }
        SaveEnv();

        State.call(4, 2);
        SaveFunction();
    }

    void TSandbox::TScript::SaveEnv() {
        State.push_global("envs");
        State.push_void((void*)this);
        State.push_value(-3);    // env
        lua_settable(State, -3); // envs[this] = env
        State.pop(1);            // remove envs from stack
    }

    void TSandbox::TScript::SaveFunction() {
        // sandbox[this] = func
        if (State.is_nil(-2)) {
            // if the first result (func) is nil then get error message from the second (err)
            const TString err(State.to_string(-1));
            State.pop(2); // remove func, err from stack
            ythrow TLuaStateHolder::TError()
                << "cannot load user script to the sandbox: "
                << err;
        } else {
            // if function was loaded successfully then save it to the table
            State.push_global("sandbox");
            State.push_void((void*)this);
            State.push_value(-4);    // func
            lua_settable(State, -3); // sandbox[this] = func
            State.pop(3);            // remove func, err and sandbox from stack
        }
    }

    TSandbox::TScript::~TScript() {
        // sandbox[this] = nil
        State.push_global("sandbox");
        State.push_void((void*)this);
        State.push_nil();
        lua_settable(State, -3);
        State.pop(1); // remove sandbox from stack

        // envs[this] = nil
        State.push_global("envs");
        State.push_void((void*)this);
        State.push_nil();
        lua_settable(State, -3);
        State.pop(1); // remove envs from stack
    }

    void TSandbox::TScript::Push() {
        // return sandbox[this]
        State.push_global("sandbox");
        State.push_void((void*)this);
        lua_gettable(State, -2);
        State.remove(-2); // remove sandbox from stack
    }

    void TSandbox::TScript::InitEnv() {
        // init_env(envs[this])
        State.push_global("envs");
        State.push_global("init_env");
        State.push_void((void*)this);
        lua_gettable(State, -3);
        State.call(1, 0);
        State.pop(1); // remove envs from stack
    }

    void TSandbox::TScript::UpdateEnv(const TStringBuf& name, const TStringBuf& object) {
        // envs[this][name] = object
        State.push_global("envs");
        State.push_void((void*)this);
        lua_gettable(State, -2);
        State.push_string(name.data());
        if (object.empty()) {
            State.push_nil();
        } else {
            State.push_global(object.data());
        }
        lua_settable(State, -3);
        State.pop(1); // remove envs[this] and envs from stack
    }

}
