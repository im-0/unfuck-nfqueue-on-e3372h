[![Build Status](https://api.travis-ci.org/im-0/unfuck-nfqueue-on-e3372h.svg?branch=master)](https://travis-ci.org/im-0/unfuck-nfqueue-on-e3372h)
# `unfuck_nfqueue.ko`

This repository contains source code of kernel module that fixes netfilter's
NFQUEUE support on Huawei E3372h LTE modem.

Huawei E3372h runs linux with some very basic Android userspace.
Unfortunately, kernel configuration prevents normal use of NFQUEUE
because internal packet queuing interfaces are locked exclusively
by compiled-in modules `ip_queue` and `ip6_queue` (-j QUEUE and libipq in
userspace).

This module just calls deinitialization functions for these deprecated
modules.

Also, this repository could be used as an example of how to build
custom kernel modules for E3372h.

## Supported firmware versions

Tested on 22.328.62.00.143 (HiLink).

## Symptoms

* `nfq_unbind_pf()` returns error, `errno` is 22 (EINVAL/Invalid argument).
* `nfq_bind_pf()` returns error, `errno` is 16 (EBUSY/Device or resource busy).
* `/proc/net/netfilter/nf_queue` indicates that IPv4 and IPv6 queues are
monopolized by `ip*_queue`:
    ```
     0 NONE
     1 NONE
     2 ip_queue
     3 NONE
     4 NONE
     5 NONE
     6 NONE
     7 NONE
     8 NONE
     9 NONE
    10 ip6_queue
    11 NONE
    12 NONE
    ```
