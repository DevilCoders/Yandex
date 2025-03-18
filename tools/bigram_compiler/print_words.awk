BEGIN {
    FS = "\t";
    OFS = "\t";
}

{
   key = $1;
   split(key, words, "_");
   for (i = 1; i <= length(words); ++i) {
        print words[i];
   }
}
