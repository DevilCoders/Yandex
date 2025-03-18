namespace NBlender::NProto;

struct TMoveToRightCommand {
    Cmd (required) : string;
    MoveIntent (required) : string;
    LeaveAsText (required) : bool;
    SetAsTextIfInRight: bool;
    MoveCondition : string;
    SearchPropsKey (required) : string;
    EncodedViewType : string;
    LeaveAsTextBySubtype : bool;
    ClearRightBeforeAdd: bool;
    WorkOnlyIfEmptyRight: bool;
    SerpInfoSubtype : string;
};

struct TPutMarkerToIntentCommand {
    Cmd (required) : string;
    If (required) : string;
    IntentName (required) : string;
    MarkerKey (required) : string;
    MarkerVal (required) : string;
    spkey (cppname=SearchPropsKey) : string;
    spval (cppname=SearchPropsVal) : string;
};

struct TNailIntentDocCommand {
    Cmd (required) : string;
    on (cppname=Enable): bool (default=true);
    PosIf : string;
    CtxIf : string;
    CollectAllIntentPos : bool (default=false);
    IntentName (required) : string;
    MoveTo (required) : any;
    RealWork : bool (default=true);
    spkey (cppname=SearchPropsKey) : string;
    spval (cppname=SearchPropsVal) : string;
    DocMarkerKey : string (default="");
    DocMarkerVal : string (default="1");
};

struct TSetIntentDocsCommand {
    Cmd (required) : string;
    IntentName (required) : string;
    ReplaceLeftIf : string;
    DeleteLeftIf : string;
    ReplaceLeftDocAttrs : string;
    AddRightIf : string;
    CopyRightFromLeft : bool;
    AddRightDocAttrs : string;
    SpKey : string;
};

struct TMakeInsertRequestCommand {
    struct TMain {
        Mode : string (required);
        PosExpr : string (required);
        ViewType : string;
        ActionPrior : double;
    };
    struct TRight {
        Mode : string (required);
        AllowedList : {string -> bool};
        ExceptList : {string -> bool};
        Requires : {string -> bool};
        PosExpr : string;
        ViewType : string;
    };
    struct TWizplace {
        Mode : string (required);
        PosExpr : string (required);
        ViewType : string;
        ActionPrior : double;
    };
    If (required) : string;
    IntentName (required) : string;
    UtilityExpr : string;
    Main : TMain;
    Right : TRight;
    Wizplace : TWizplace;
    SpKey : string;
};

struct TSetIntentLocalRandomCommand {
    Cmd (required) : string;
    If (required) : string;
    Intent (required) : string;
    Fml (required) : string;
    spkey : string;
    spval : string;
};
