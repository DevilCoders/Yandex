namespace NBlender::NProto;

struct TProtectedTop {
    top (cppname = Top, required) : ui32;
    trusted_intents (cppname = TrustedIntents) : {string -> bool};
};
