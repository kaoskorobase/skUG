"""SCons.Tool.faust

Tool for building faust dsp files.

There normally shouldn't be any need to import this module directly.
It will usually be imported through the generic SCons.Tool.Tool()
selection method.

"""

#
# Copyright (c) 2008 Stefan Kersten
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
# KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

import os
import os.path
import re
import string

import SCons.Action
import SCons.Builder
import SCons.Defaults
import SCons.Environment
import SCons.Tool
import SCons.Util

FAUST = 'faust'
INCLUDE_RE = re.compile(r'import\s*\(\s*"([^"]+)"\s*\)\s*;', re.M)

def faust_scan(node, env, path):
   contents = node.get_contents()
   includes = INCLUDE_RE.findall(contents)
   return includes

def generate(env):
    dsp = SCons.Builder.Builder(
            action = 'faust -a ${FAUST_ARCHITECTURE}.cpp -o $TARGET $SOURCE',
            suffix = '.cpp',
            src_suffix = '.dsp')
    xml = SCons.Builder.Builder(
            action = ['faust -o /dev/null -xml $SOURCE', SCons.Defaults.Move('$TARGET', '${SOURCE}.xml')],
            suffix = '.dsp.xml',
            src_suffix = '.dsp')
    sc = SCons.Builder.Builder(
            action = '$FAUST2SC --prefix="${FAUST_PREFIX}" -o $TARGET $SOURCE',
            suffix = '.sc',
            src_suffix = '.dsp.xml')

    env.Append(BUILDERS = { 'Faust' : dsp,
                            'FaustXML' : xml,
                            'FaustSC' : sc })

    env['FAUST_ARCHITECTURE'] = 'module'
    env['FAUST_PREFIX'] = ''

    env.Append(SCANNERS = env.Scanner(function = faust_scan, skeys = ['.dsp']))

def exists(env):
    return (env.WhereIs(FAUST) or SCons.Util.WhereIs(FAUST))
