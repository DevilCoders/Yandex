namespace NTranslateWizRule;

struct TTranslateWiz {
    Enabled : bool(default = false);
    TargetGrouping : string(default = "translate");
    Intent : string(default="WIZTRANSLATE");
    MinPos : ui32(default = 15);
};
