%%{
    machine CharacterNames;

    yc_sp = yc_gen_09 | yc_gen_0A | yc_gen_B6 | yc_gen_0D | yc_gen_20; # [\t\n\v\f\r ]

    EOF = 0; # end of stream marker (we'll remove this requirement with advance to rl6's eof)

    # character synonyms
    yc_zero          = 0x0000; # '\0';
    yc_tab           = 0x0009; # '\t';

    yc_space         = 0x0020; # ' ';
    yc_exclamation   = 0x0021; # '!';
    yc_quotation     = yc_gen_22; # '"';
    yc_number_sign   = 0x0023; # '#';
    yc_dollar        = 0x0024; # '$';
    yc_percent       = 0x0025; # '%';
    yc_ampersand     = 0x0026; # '&';
    yc_apostrophe    = yc_gen_27; # '\'';
    yc_left_paren    = 0x0028; # '(';
    yc_right_paren   = 0x0029; # ')';
    yc_asterisk      = 0x002A; # '*';
    yc_plus          = 0x002B; # '+';
    yc_comma         = 0x002C; # ',';
    yc_minus         = 0x002D; # '-';
    yc_dot           = 0x002E; # '.';
    yc_slash         = 0x002F; # '/';

    yc_colon         = 0x003A; # ':';
    yc_less          = 0x003C; # '<';
    yc_equals        = 0x003D; # '=';
    yc_greater       = 0x003E; # '>';

    yc_at_sign       = 0x0040; # '@';

    yc_left_bracket  = 0x005B; # '[';
    #yc_backslash    = 0x005C; # '\\';
    yc_right_bracket = 0x005D; # ']';
    yc_caret         = 0x005E; # '^';
    yc_underscore    = 0x005F; # '_';

    yc_accent        = yc_60; # '`';

    yc_left_brace    = 0x007B; # '{';
    yc_vert_bar      = 0x007C; # '|';
    yc_right_brace   = 0x007D; # '}';
    yc_tilde         = 0x007E; # '~';
}%%
