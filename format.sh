#!/bin/bash

find include -regex '.*\.\(cpp\|h\)' -exec clang-format -style='file:.clang-format' -i {} \;
find test -regex '.*\.\(cpp\|h\)' -exec clang-format -style='file:.clang-format' -i {} \;
find src -regex '.*\.\(cpp\|h\)' -exec clang-format -style='file:.clang-format' -i {} \;


