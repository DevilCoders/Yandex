#!/bin/sh
java -cp $(ls -1 magic/*.jar | tr "\n" ":") MagicKt
