#!/usr/bin/python
#
# Processing tool for kernel config files

# Copyright 2006-2010 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
# Licensed under the Amazon Software License (the "License").  You may not use
# this file except in compliance with the License. A copy of the License is
# located at http://aws.amazon.com/asl or in the "license" file accompanying
# this file.  This file is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
# OR CONDITIONS OF ANY KIND, either express or implied. See the License for
# the specific language governing permissions and limitations under the License.
#
# Author: Cristian Gafton <gafton@amazon.com>

import os
import re
import sys
import optparse

class ConfigFile(object):
    """ holds data about a config file """
    def __init__(self, filename=None):
        self.filename = filename
        self._options = []
        self._data = {}
        self._reon = re.compile("^(?P<opt>CONFIG_\w+)=.*$")
        self._reoff = re.compile("^# (?P<opt>CONFIG_\w+) is not set$")
        if self.filename:
            self._load()

    def _load(self):
        f = open(self.filename, "r")
        for line in f.readlines():
            line = line.strip()
            m = self._reon.match(line)
            if not m:
                m = self._reoff.match(line)
            if not m:
                continue
            opt = m.groupdict().get("opt", None)
            if opt is None:
                raise RuntimeError, "Could not parse line:", line
            self.add_line(opt, line)
        f.close()
    def has_option(self, opt):
        return opt in self._options
    def has_line(self, line):
        return line in self._data.values()
    def get_line(self, opt):
        return self._data.get(opt, None)
    def add_line(self, opt, line):
        if opt not in self._options:
            self._options.append(opt)
        self._data[opt] = line
    def iteroptions(self):
        for opt in self._options:
            yield opt
    def iterlines(self):
        for opt in self._options:
            yield self._data[opt]
    # major mode operations we know how to do
    def common(self, other):
        ret = ConfigFile()
        for opt in self.iteroptions():
            line = self.get_line(opt)
            if other.has_line(line):
                ret.add_line(opt, line)
        return ret
    def update(self, other):
        ret = ConfigFile()
        # add/update all options from file1
        for opt in self.iteroptions():
            if other.has_option(opt):
                ret.add_line(opt, other.get_line(opt))
            else:
                ret.add_line(opt, self.get_line(opt))
        return ret
    def merge(self, other):
        # a merge is basically an update of file1 followed by add extra stuff from file2...
        ret = self.update(other)
        # and now add/fill all options from file2 that have not been processed
        # in file1 already
        for opt in other.iteroptions():
            if not self.has_option(opt):
                ret.add_line(opt, other.get_line(opt))
        return ret
    def diff1(self, other):
        """settings from self which are missing/different from other"""
        ret = ConfigFile()
        for opt in self.iteroptions():
            line1 = self.get_line(opt)
            line2 = other.get_line(opt)
            if line1 != line2:
                ret.add_line(opt, line1)
        return ret
    def diff2(self, other):
        """settings from other which are missing/different from self"""
        return other.diff1(self)
    def changed(self, other):
        """settings that are present in both configs, but have different values"""
        for opt in self.iteroptions():
            if other.has_option(opt):
                line1 = self.get_line(opt)
                line2 = other.get_line(opt)
                if line1 != line2:
                    print "- %s" % (line1,)
                    print "+ %s" % (line2,)
                    print
        return ConfigFile()
        

def main():
    usage = """%prog <--MODE> FILE1.config FILE2.config"""
    def record_mode(option, opt_str, value, parser):
        opt = opt_str[2:]
        if parser.values.mode is not None:
            parser.print_help()
            raise optparse.OptionValueError("can not use %s with --%s" % (opt_str, parser.values.mode))
        parser.values.mode = opt
    parser = optparse.OptionParser(
        usage=usage,
        description="process kernel config options from FILE1 and FILE2 using MODE")
    parser.set_defaults(mode=None)
    parser.add_option("--merge", help="print union of FILE1 and FILE2, with FILE2 overrides settings from FILE1",
                      action="callback", callback=record_mode)
    parser.add_option("--update", help="print FILE1 only updated with settings from FILE2",
                      action="callback", callback=record_mode)
    parser.add_option("--common", help="print same value settings from FILE1 and FILE2",
                      action="callback", callback=record_mode)
    parser.add_option("--diff1", help="print FILE1 settings that are different/not present in FILE2",
                      action="callback", callback=record_mode)
    parser.add_option("--diff2", help="print FILE2 settings that are different/not present in FILE1",
                      action="callback", callback=record_mode)
    parser.add_option("--changed", help="print diff for common settings in FILE1 and FILE2",
                      action="callback", callback=record_mode)
    (options, args) = parser.parse_args()
    if len(args) != 2:
        parser.print_help()
        parser.error("need 2 files to do work")
    if not options.mode:
        parser.print_help()
        parser.error("you need to specify a mode of operation")
    file1 = ConfigFile(args[0])
    file2 = ConfigFile(args[1])
    func = getattr(file1, options.mode, None)
    if func is None:
        raise RuntimeError("Oops, looks like somebody forgot to implement the %s method in ConfigFile" % (options.mode,))
    ret = func(file2)
    for line in ret.iterlines():
        print line
    
if __name__ == '__main__':
    main()
