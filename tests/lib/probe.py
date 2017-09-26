#!/usr/bin/env python

"""
Copyright (c) 2012-2015, Mike Smith, <msmith@purgatory.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

o Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

o Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

import sys
import os
import subprocess
import re

class ProbeException(Exception):
    pass

class NodeException(Exception):
    pass

class node(object):

    def __init__(self, probe, address, function, description):
        self._probe = probe
        self.address = address
        self._function = function
        self._description = description

    def _node_cmd(self, verb, args):
        return self._probe.node_cmd(self, [verb, "-n", self.address] + args)

    def reset_to_defaults(self):
        self._node_cmd("load_params", ["-d"])

    def get_param(self, param_name):
        value = self._node_cmd("param", [param_name])
        print value
        info = value.split()
        if len(info) == 5:
            return info[4]  # return encoded value
        else:
            return info[1]  # return plain numeric value

    def set_param(self, param_name, param_value):
        self._node_cmd("param", [param_name, param_value])


class probe(object): 

    def __init__(self, toolpath = "../build/lintool/lintool"):
        if not os.path.isfile(toolpath) or not os.access(toolpath, os.X_OK):
            raise ProbeException("tool not found at %s" % toolpath)
        self._toolpath = toolpath

    def _cmd(self, args):
        '''return the result of invoking the tool with the supplied arguments as a list of lines'''
        if not isinstance(args, list):
            raise RuntimeError("args not list")
        str_args = [str(elt) for elt in args]
        result = subprocess.check_output([self._toolpath] + str_args)

        return result.split("\n")

    def node_cmd(self, node, args = []):
        return self._cmd(args)

    def scan(self, node_address = None):
        args = ["scan"]
        if node_address is not None:
            args += ["-n", str(node_address)]

        nodes = list();
        lines = self._cmd(args)
        for line in lines:
            words = line.split("/")
            if len(words) == 3:
                nodes.append(node(self, words[0], words[1], words[2]))

        if node_address is not None:
            if len(nodes) > 1:
                raise ProbeException("too many nodes returned scanning for an individual")
            if len(nodes) == 0:
                raise ProbeException("node not found")
            return nodes[0]

        return nodes
