#!/bin/bash

#cluster="`dirname "$0"`"
#cluster="`cd "$cluster"; pwd`"
build=${cluster}/../../build

time ${build}/benchmarks/word_count/word_count '/home/kit/stud/uagtc/common/inputs/RC_2015-01.body' '/home/kit/stud/uagtc/outputs/reddit-$-##'
