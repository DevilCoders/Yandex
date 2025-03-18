namespace NMonitorSc;


struct TMonitoringWizardCondition (strict) {
    attribute : string (required, cppname = Attribute, allowed = ["_Markers", "_SerpInfo", "ServerDescr"]);
    path : string (cppname = Path);
    special_method : string (cppname = SpecialMethod, allowed = ["", "contains"]);
    result : string (required, cppname = Result);

    (validate) {
        if ((Attribute() == "_SerpInfo"sv) && !HasPath())
            AddError("Path should be defined if you use \"_SerpInfo\" attribute");
        if ((Attribute() == "_Markers"sv) && HasPath())
            AddError("Path should not be defined if you yse \"_Markers\" attribute");
    };
};

struct TMonitoringWizardDescription {
    description (required, cppname = Description) : { string -> [TMonitoringWizardCondition] };
    signals (required, cppname = Signals) : {string -> [string]};
    notifications (cppname = Notifications) : {string -> {string -> {string -> [string]}}};

    (validate) {
        for (const auto& signal : Signals()) {
            if (!helper->ValidateSlice(signal.Key(), helper->WIZARD))
                AddError("Each signal\'s slice should have got 4 slice. Check format \"platform-device-region-column\".");
        }
        for (const auto& slice : Signals()) {
            for (const auto& signal : slice.Value()) {
                if (Description(signal).IsNull())
                    AddError("Each wizard should have got a description");
            }
        }
    };
};

struct TEventDescription (strict) {
    handler (required, cppname = Handler) : string;
    signals (required, cppname = Signals) : { string -> [ string ] };
    is_filter_event (cppname = IsFilterEvent) : bool (default = false);
    depends_on_intent (cppname = DependsOnIntent) : bool (default = true);
    depends_on_grouping (cppname = DependsOnGrouping) : string;

    (validate) {
        for (const auto& signal : Signals()) {
            if (!helper->ValidateSlice(signal.Key(), helper->EVENT))
                AddError("Invalid format for event\'s slice.");
        }
    };
};

struct TPropsDescription {
    name (required, cppname = Name)            : string;
    property (required, cppname = Property)    : string;
    result (required, cppname = Result)        : string;
    slices (required, cppname = Slices)        : [ string ];

    (validate) {
        for (const auto& slice: Slices()) {
            if (!helper->ValidateSlice(slice, helper->PROP))
                AddError("Invalid format for prop\'s slice.");
        }
    };
};

struct TConfig {
    clients (required, cppname = Clients) : { string -> { string -> [ string ] } };
    wizards (required, cppname = Wizards) : TMonitoringWizardDescription;
    events   (required, cppname = Events) : [ TEventDescription ];
    props     (required, cppname = Props) : [ TPropsDescription ];

    prefix  (required, cppname = Prefix)  : string;

    (validate) {
        for (const auto& signal: Clients()) {
            if (!helper->ValidateSlice(signal.Key(), helper->CLIENT))
                AddError("Invalid format for signal\'s slice.");
            for (const auto& client: signal.Value()) {
                for (const auto& response: client.Value()) {
                    if (!helper->ValidateResponseType(response)) {
                        AddError("Invalid name for client\'s response");
                    }
                }
            }
        }
    };
};
