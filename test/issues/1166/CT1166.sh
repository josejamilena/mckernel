#!/bin/sh
USELTP=0
USEOSTEST=0

BOOTPARAM="-c 1-3 -m 1G"
. ../../common.sh

################################################################################
$MCEXEC ./CT1166
