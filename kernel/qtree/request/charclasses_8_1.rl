%%{

machine CharacterClasses8;
alphtype unsigned short;

include CharacterClassesGen "charclasses_16.gen.rl";

#############################################
# Named Characters

cc_zero = yc_gen_00; # (EOF) [\0]
cc_tab = yc_gen_09; # [\t]
cc_linefeed = yc_gen_0A; # [\n]
cc_carriagereturn = yc_gen_0D; # [\r]
cc_space = yc_gen_20; # [ ]
cc_quotationmark = yc_gen_22; # ["]
cc_numbersign = yc_gen_23; # [#]
cc_dollarsign = yc_gen_24; # [$]
cc_percent = yc_gen_25; # [%]
cc_ampersand = yc_gen_26; # [&]
cc_apostrophe = yc_gen_27; # [']
cc_asterisk = yc_gen_2A; # [*]
cc_plus = yc_gen_2B; # [+]
cc_comma = yc_gen_2C; # [,]
cc_minus = yc_gen_2D; # [-]
cc_dot   = yc_gen_2E; # [.]
cc_slash = yc_gen_2F; # [/]
cc_digit = yc_gen_31; # [1]
cc_atsign = yc_gen_40; # [@]
cc_capitalalpha = yc_gen_41; # [A]
cc_underscore = yc_gen_5F; # [_]
cc_smallalpha = yc_gen_61; # [a]
cc_accent = yc_gen_80;
cc_unicasealpha = yc_gen_81; # georgian, hebrew, arabic alphabets
cc_softhyphen = yc_gen_8F;
cc_ideograph = yc_gen_9F;
cc_nbsp = yc_gen_A0;
cc_sectionsign = yc_gen_A7;
cc_copyrightsign = yc_gen_A9;
cc_special = yc_gen_B0;

cc_math = yc_gen_C0;
cc_math_non_ascii = yc_gen_D0;
cc_currency_non_ascii = yc_gen_D1;
cc_special_non_ascii = yc_gen_D2;

#############################################
# Classes

# = yc_gen_B1;
cc_openpunct = yc_gen_B2 | # [(\[{]
    cc_apostrophe | cc_quotationmark; # opening punctuation
cc_clospunct = yc_gen_B3 | # [)\]}]
    cc_apostrophe | cc_quotationmark; # closing punctuation
cc_surrogatelead = yc_gen_B4;
cc_surrogatetail = yc_gen_B5;
cc_whitespace = yc_gen_B6 | cc_tab | cc_linefeed | cc_carriagereturn | cc_space; # [\t\n\v\f\r ]
cc_numerosign = yc_gen_B7; # unicode yc_gen_2116
# = yc_gen_B8;
# = yc_gen_B9;
cc_cjk_termpunct = yc_gen_BA; # fullwidth cjk terminating punctuation
cc_termpunct = yc_gen_BB | cc_dot; # terminating punctuation [!.?] | [!.;?]
cc_currency = cc_dollarsign;
cc_control = yc_gen_BD | # yc_gen_01 - yc_gen_1F, yc_gen_7F excluding
    cc_tab | cc_linefeed | cc_carriagereturn;
cc_misctext = yc_gen_BE | cc_math | # [:;<=>\^`|~]
    cc_control | cc_whitespace | cc_comma | cc_asterisk | cc_ampersand |
    cc_termpunct | cc_openpunct | cc_clospunct | cc_numbersign | cc_currency | cc_percent |
    cc_plus | cc_minus | cc_dot | cc_slash | cc_atsign | cc_underscore;

cc_unknown = yc_gen_FF;

}%%

