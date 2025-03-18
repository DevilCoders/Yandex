#!/bin/sh

# .idl files are searched from here (paths must begin with framework name!)
idl_search_path="idl"

framework_search_path="framework"
proto_search_path="proto"

out_dir="out" # generator output is generated here (goes inside .gitignore)

../bin/tools-idl-app -I idl \
              -F framework \
              --in-proto-root proto \
              --out-base-root $out_dir/base \
              --out-android-root $out_dir/android \
              --out-ios-root $out_dir/ios \
              foobar/num_gen.idl
