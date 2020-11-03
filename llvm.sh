#!/bin/bash
# a compile script for llvm users
OBJCOPY=llvm-objcopy CC=clang LD=ld.lld make "$@"