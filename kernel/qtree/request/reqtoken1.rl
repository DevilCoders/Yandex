%%{
    machine ReqToken;

    action makeMUST {
        PRINT_TOKEN("makeMUST", fc);
        if (p == SepEnd) {
            this->FoundReqLanguage |= (p + 1) != eof && p[1] != L' ';
            LE.Necessity = nMUST;
            ++SepEnd;
        }
    }
    action makeMUSTNOT {
        PRINT_TOKEN("makeMUSTNOT", fc);
        if (p == SepEnd) {
            FoundReqLanguage |= (p + 1) != eof && p[1] != L' ';
            LE.Necessity = nMUSTNOT;
            ++SepEnd;
        }
    }
    action makeExactWord {
        PRINT_TOKEN("makeExactWord", fc);
        if (p == SepEnd) {
            FoundReqLanguage |= (p + 1) != eof && p[1] != L' ' && p[1] != L'!'; // !! operator is handled in makeExactLemma
            LE.FormType = fExactWord;
            ++SepEnd;
        }
    }
    action makeExactLemma {
        PRINT_TOKEN("makeExactLemma", fc);
        if (p == SepEnd) {
            FoundReqLanguage |= (p + 1) != eof && p[1] != L' ';
            LE.FormType = fExactLemma;
            ++SepEnd; // +1 because for '!!' makeExactWord works first
        }
    }
    action clear_idf {
        LE.Idf = 0;
    }
    action update_idf {
        LE.Idf = LE.Idf * 10 + (fc - '0');
    }
    action clear_softness {
        ResetSoftness();
    }
    action update_softness {
        UpdateSoftness(fc);
    }
    action begin_entry {
        LE.EntryLeng = 0;
        LE.EntryPos = (const wchar16*)fpc - LE.Text;
    }
    action update_entry {
        ++LE.EntryLeng;
    }

    include Symbols "symbols1.rl";

    necesschar  = '+' @makeMUST | '-' @makeMUSTNOT;
    formchar    = '!' @makeExactWord | '!!' @makeExactLemma;
    prefix      = ( ( necesschar formchar? ) | ( formchar necesschar? ) )**;

    integer  = digit+;
    idf      = ( yc_colon yc_colon ) ( integer >clear_idf      $update_idf );
    softness = ( yc_slash yc_slash ) ( integer >clear_softness $update_softness );
    postfix  = ( idf? );

    identdelim = ( yc_minus | yc_plus | yc_underscore | yc_slash | yc_at_sign ); # [-+_/@];
    tokalpha = yalpha ( accent | softhyphen )?;
    tokchar = yalpha;
    include MultitokenDef "../../../library/cpp/tokenizer/multitoken_v3.rl";
    identpart = ( yalnum+ | tokalpha+ ( tokdelim tokalpha+ )* toksuffix? );
    ident = ( ( ( identpart identdelim ) | ( yalnum* digit yc_dot ) )* identpart );

    sep = ( yc_space | yc_tab | yc_lf | yc_cr ); # [ \t\n\r];



}%%
