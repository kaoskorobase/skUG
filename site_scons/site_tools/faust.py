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
import SCons.Scanner
import SCons.Tool
import SCons.Util

FAUST = 'faust'
INCLUDE_RE = re.compile(r'import\s*\(\s*"([^"]+)"\s*\)\s*;', re.M)

def flatten(l):
    r = []
    for ll in l:
        r += ll
    return r

def dsp_source_scanner(node, env, path):
    """Scan source files for imported files in `path'."""
    contents = node.get_contents()
    includes = INCLUDE_RE.findall(contents)
    path = [os.path.dirname(str(node))] + list(path)
    return filter(os.path.exists, flatten(map(lambda f: map(lambda d: os.path.join(os.path.realpath(str(d)), f), path), includes)))

def dsp_target_scanner(node, env, path):
    """Search for architecture file in `path'."""
    arch = env['FAUST_ARCHITECTURE'] + '.cpp'
    return filter(os.path.exists, map(lambda d: os.path.join(str(d), arch), path))

def svg_emitter(target, source, env):
    target = target + map(lambda x: os.path.join(str(x), 'process.svg'), target)
    print map(str, target)
    return (target, source)
    #return (target + [os.path.join(str(target[0]), 'process.svg')], source)

def svg_scanner(node, env, path):
    return [os.path.join(str(node), 'process.svg')]

def generate(env):
    dsp = SCons.Builder.Builder(
            action = 'faust ${FAUST_FLAGS} -a ${FAUST_ARCHITECTURE}.cpp -o $TARGET $SOURCE',
            suffix = '.cpp',
            src_suffix = '.dsp',
            target_scanner = env.Scanner(
                function = dsp_target_scanner,
                path_function = SCons.Scanner.FindPathDirs('FAUST_PATH')))
    xml = SCons.Builder.Builder(
            action = ['faust ${FAUST_FLAGS} -o /dev/null -xml $SOURCE', SCons.Defaults.Move('$TARGET', '${SOURCE}.xml')],
            suffix = '.dsp.xml',
            src_suffix = '.dsp')
    svg = SCons.Builder.Builder(
            action = ['faust ${FAUST_FLAGS} -o /dev/null -svg $SOURCE', SCons.Defaults.Move('$TARGET', '${SOURCE}-svg')],
            suffix = '.dsp-svg',
            src_suffix = '.dsp',
            single_source = True,
            target_factory = env.Dir,
            target_scanner = env.Scanner(function = svg_scanner))
    sc  = SCons.Builder.Builder(
            action = '$FAUST2SC --lang=sclang --prefix="${FAUST2SC_PREFIX}" -o $TARGET $SOURCES',
            suffix = '.sc',
            src_suffix = '.dsp.xml',
            multi = True)
    hs  = SCons.Builder.Builder(
            action = '$FAUST2SC --lang=haskell --prefix="${FAUST2SC_HASKELL_MODULE}" -o $TARGET $SOURCES',
            suffix = '.hs',
            src_suffix = '.dsp.xml',
            multi = True)

    env.Append(BUILDERS = { 'Faust'         : dsp,
                            'FaustXML'      : xml,
                            'FaustSVG'      : svg,
                            'FaustSC'       : sc,
                            'FaustHaskell'  : hs })

    env['FAUST_ARCHITECTURE']       = 'module'
    env['FAUST_FLAGS']              = []
    env['FAUST_PATH']               = ['.', '/usr/local/lib/faust', '/usr/lib/faust']
    env['FAUST2SC']                 = 'faust2sc'
    env['FAUST2SC_PREFIX']          = ''
    env['FAUST2SC_HASKELL_MODULE']  = ''
    
    env.Append(SCANNERS = [
        env.Scanner(function = dsp_source_scanner,
                    skeys = ['.dsp'],
                    path_function = SCons.Scanner.FindPathDirs('FAUST_PATH'))
        ])

def exists(env):
    return (env.WhereIs(FAUST) or SCons.Util.WhereIs(FAUST))
