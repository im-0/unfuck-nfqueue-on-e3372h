[![Build Status](https://api.travis-ci.org/im-0/unfuck-nfqueue-on-e3372h.svg?branch=master)](https://travis-ci.org/im-0/unfuck-nfqueue-on-e3372h)
# unfuck_nfqueue.ko

This repository contains source code of kernel module that fixes netfilter's
NFQUEUE support on Huawei E3372h LTE modem.

Huawei E3372h runs linux with some very basic Android userspace.
Unfortunately, kernel configuration prevents normal use of NFQUEUE
because internal packet queuing interfaces are locked exclusively
by compiled-in modules ip_queue and ip6_queue (-j QUEUE and libipq in
userspace).

This module just calls deinitialization functions for these deprecated
modules.

Also, this repository could be used as an example of how to build
custom kernel modules for E3372h.
