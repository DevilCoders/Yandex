#include <stdio.h>
#include <iostream>

#include <util/string/strip.h>

#include "trans_str.h"

using namespace std;

int main(int argc, char** argv)
{
   if(argc!=2)
   {
      cout << "Usage: " << argv[0] << " cfg-file " << endl;
      return 1;
   }

   tsStringTransformer ST;

   if(!ST.prepare(argv[1], true)) //with debug information!!!
   {
      cout << "Wrong settings, exited." << endl;
      return 1;
   }

   while(ST.isOK())
   {
      char buf[0x1000];
      cin.getline(buf, 0x1000);
      if(cin.eof())
         break;
      buf[cin.gcount()]=0;
      TString str(buf);
      str = StripInPlace(str);
      if(str[0]=='#' || !str[0])
         continue;
      cout << "Input  link: " << str.c_str() << endl;
      ST.process(str);
      cout << "Result link: " << str.c_str() << endl;
   }

   return 0;
}
