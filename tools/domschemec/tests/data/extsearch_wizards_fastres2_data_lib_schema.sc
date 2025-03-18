namespace NDataLib::NSchema;

struct TWizardMetaData {
    author : string (required, != "");
    human_readable_name : string (required, != "");
    state : string (allowed = ["disabled", "paused", "enabled"], default = "disabled");
};

// There is only one TWizardsList stored under key "wizards_list"
struct TWizardsList {
    wizards : {string -> TWizardMetaData};
};

struct TWizardVersion {
    values : {string -> string}; // key -> hash of content. Content itself is stored in key "blob/<hash>"
};

// For each enabled key in "wizards_list/wizards" there is supposed to be a wizard state node with key "wizards/<id>"
struct TWizard {
    current_version : TWizardVersion;
    next_version : TWizardVersion;

    // TODO: staging for editing in web-interface
};
