%%{
    machine Symbols;

    include CharacterClasses "charclasses_16.rl";
    include CharacterClasses8 "charclasses_8_1.rl";

    include CharacterNames   "charnames.rl";
    #
    # CODES_YANDEX symbols
    #

    accent         = cc_accent;
    softhyphen     = cc_softhyphen;
    ideograph      = cc_ideograph;
    nbsp           = cc_nbsp;
    section        = cc_sectionsign;
    degree         = cc_special;
    numero         = cc_numerosign;
    surrogatelead  = cc_surrogatelead;
    surrogatetail  = cc_surrogatetail;

    yc_lf = cc_linefeed; # \n
    yc_cr = cc_carriagereturn; # \r

    yspecial = accent | softhyphen | ideograph | nbsp | section | degree | numero;
    yspecialkey = cc_math_non_ascii | cc_currency_non_ascii | cc_special_non_ascii | cc_numerosign | cc_copyrightsign;

    ylatinsmall =    yc_61 | yc_62 | yc_63 | yc_64 | yc_65 | yc_66 | yc_67 |
         yc_68 | yc_69 | yc_6A | yc_6B | yc_6C | yc_6D | yc_6E | yc_6F |
         yc_70 | yc_71 | yc_72 | yc_73 | yc_74 | yc_75 | yc_76 | yc_77 |
         yc_78 | yc_79 | yc_7A ;

    ydigit = cc_digit;

    ycapital = cc_capitalalpha;

    ysmall = cc_smallalpha;

    yalpha       = ycapital | ysmall | cc_unicasealpha;
    yalnum       = ydigit | yalpha;

    ytitle   = ydigit | ycapital;  # may be at the beginning of sentence

    # Multitoken composition: delimiters and suffixes
    tokdelim    = cc_apostrophe | cc_minus;   # [\'\-] TODO: add yc_underscore [_]
    tokprefix   = cc_numbersign | cc_atsign | yc_dollar; # [#@$]

    # 1..31 | termpunct | [ \"#\$%&\'()*+,\-/;<=>@\[\\\]\^_\`{|}~] | 0x7F | yspecial
    # yc_07 and yc_1B do not exist
    miscnlp = (cc_nbsp | cc_misctext | yspecial) - yspecialkey;

    # fallback
    othermisc = any - yalnum - yc_zero - miscnlp - yspecialkey;
}%%
