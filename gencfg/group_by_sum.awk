  { a[$1] += $2 }
  END {
    for (i in a) {
      printf "%s %s\n", i, a[i];
    }
  }
