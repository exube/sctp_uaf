#!/bin/bash

CC="gcc"
CCOPTS="-pthread -g"
LDOPTS="-lsctp -g"
OUTPUT="sctp_uaf"

clear
$CC $CCOPTS main.c $LDOPTS -o $OUTPUT
$CC $CCOPTS spam.c $LDOPTS -o ${OUTPUT}_spam
