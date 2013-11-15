#!/bin/bash

find . -name '.sconsign' \
   -or -name '*.o'       \
   -or -name '*.a'       \
   -or -name '*.so'      \
   -or -name '*.os'      \
   -or -name '*.pyc'      \
   -or -name '*~'        \
   -or -name '*.tmp'     \
   | xargs rm

rm -rf inc/ lib/ bin/
