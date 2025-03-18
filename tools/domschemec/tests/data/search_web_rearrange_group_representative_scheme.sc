namespace NGroupRepresentative;

struct TParameters {
    Enabled : bool (default = true);
    DryRun : bool (default = false);
    Verbosity : i32 (default = 0);
    SkipUrlParams : i32 (default = 2);
    AvoidDocPfd : i32 (default = 1);
    FirstGroupOnly : bool (default = false);
    SwapDuplicates : bool (default = true);
    UseExtendedLanguages : bool (default = true);
    KeepDuplicatedGroups : bool (default = false);
    EraseDocs : bool (default = true);
    SecondPass : bool (default = true);
    UseInterfaceLanguage : bool (default = true);
    UseEnglishIfNoneMatch : bool (default = true);
    PreferEnglish : bool (default = false);
};
