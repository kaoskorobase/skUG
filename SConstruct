# -*- python -*- =======================================================
# FILE:         SConstruct
# CONTENTS:     scons build script for skug
# AUTHOR:       steve AT k-hornz DOT de
# ======================================================================

import glob
import os
import re
import tarfile
import time

# ======================================================================
# Setup
# ======================================================================

EnsureSConsVersion(0, 96)
EnsurePythonVersion(2, 3)
SConsignFile()

# ======================================================================
# Constants
# ======================================================================

PACKAGE = 'vep'
VERSION = '0.1'

# ======================================================================
# Util
# ======================================================================

def set_platform(env, platform, cpu):
    env['PLATFORM']             = platform
    env['CPU']                  = cpu
    if platform == 'darwin':
        env['PLATFORM_SYMBOL']  = 'SC_DARWIN'
        env['PLUGIN_EXT']       = '.scx'
    elif platform == 'linux':
        env['PLATFORM_SYMBOL']  = 'SC_LINUX'
        env['PLUGIN_EXT']       = '.so'
    elif platform == 'windows':
        env['PLATFORM_SYMBOL']  = 'SC_WIN32'
        env['PLUGIN_EXT']       = '.scx'
    else:
        print 'Unknown platform: %s' % PLATFORM
        Exit(1)

def make_os_env(*keys):
    env = os.environ
    res = {}
    for key in keys:
        if env.has_key(key):
            res[key] = env[key]
    return res

# ======================================================================
# Command line options
# ======================================================================

opts = Options('scache.conf', ARGUMENTS)
opts.AddOptions(
    BoolOption('ALTIVEC',
               'Build with Altivec support', 1),
    ('CUSTOMCCFLAGS', 'Custom c compiler flags', ''),
    ('CUSTOMCXXFLAGS', 'Custom c++ compiler flags', ''),
    BoolOption('BENCHMARK',
               'Build with benchmarking instrumentation', 0),
    BoolOption('DEBUG',
               'Build with debugging information', 0),
    ('OPTFLAGS',
               'Optimization flags',
               ' '.join(['-O2', '-ffast-math', '-funroll-loops', '-fstrength-reduce', '-ftree-vectorize'])),
    PathOption('SC_SOURCE_DIR',
               'SuperCollider source directory', '../supercollider/'),
    BoolOption('SSE',
               'Build with SSE support', 1),
    ('CROSSCOMPILE', 'Platform to crosscompile for', 'None')
    )

# ======================================================================
# Base environment
# ======================================================================

env = Environment(options = opts,
                  PACKAGE = PACKAGE,
                  VERSION = VERSION,
                  ENV = make_os_env('PATH', 'PKG_CONFIG_PATH'))

# Tools
env.Tool('faust')

# Cross compilation
if env['CROSSCOMPILE'] != 'None':
    if env['CROSSCOMPILE'] == 'mingw':
        set_platform(env, 'windows', 'i386')
        env.Tool('crossmingw')
    else:
        print 'Unknown cross compilation environment: %s' % env['CROSSCOMPILE']
        Exit(1)
else:
    set_platform(env, os.uname()[0].lower(), os.uname()[4].lower())

# Custom compiler flags
env.Append(
    CCFLAGS = env['CUSTOMCCFLAGS'],
    CXXFLAGS = env['CUSTOMCXXFLAGS'])

# Defines and compiler flags
env.Append(
    CPPDEFINES = ['_REENTRANT', env['PLATFORM_SYMBOL']],
    CCFLAGS = ['-Wno-unknown-pragmas'],
    CXXFLAGS = ['-Wno-deprecated'],
    CPPPATH = map(lambda f: os.path.join(env['SC_SOURCE_DIR'], 'headers', f),
                  ['common', 'plugin_interface', 'server']))

# Benchmarking
if env['BENCHMARK']:
    env.Append(CPPDEFINES = ['VEP_BENCHMARK'])

# Optimization flags
# FIXME: Omitting -O2 crashes Faust plugins during initialization.
env.Append(CCFLAGS = ['$OPTFLAGS'])

# Debugging flags
if env['DEBUG']:
    env.Append(CCFLAGS = '-g')
else:
    env.Append(CPPDEFINES = ['NDEBUG'])

# Platform specific
if env['CPU'] == 'ppc':
    env.Append(CCFLAGS = '-fsigned-char')

if env['ALTIVEC'] and (env['CPU'] == 'ppc'):
    altiEnv = env.Copy()
    altiEnv.Append(CCFLAGS = ['-maltivec'])
    altiConf = Configure(altiEnv)
    has_vec = altiConf.CheckCHeader('altivec.h')
    altiConf.Finish()
    if has_vec:
        env.Append(CCFLAGS = ['-maltivec', '-mabi=altivec'])
elif env['SSE'] and re.match("^i?[0-9x]86", env['CPU']):
    sseEnv = env.Copy()
    sseEnv.Append(CCFLAGS = ['-msse2'])
    sseConf = Configure(sseEnv)
    has_vec = sseConf.CheckCHeader('xmmintrin.h')
    sseConf.Finish()
    if has_vec:
        env.Append(CCFLAGS = ['-msse', '-mfpmath=sse'])

# Finish
opts.Save('scache.conf', env)
Help(opts.GenerateHelpText(env))

# ======================================================================
# Plugins
# ======================================================================

def make_plugin_target(env, dir, name):
    if env['PLATFORM'] == 'darwin':
        pdir = 'osx'
    elif env['PLATFORM'] == 'linux':
        pdir = 'linux'
    elif env['PLATFORM'] == 'windows':
        pdir = 'windows'
    else:
        pdir = env['PLATFORM']
    return os.path.join(dir, pdir, name)

def make_faust_plugin_target(env, dir, src):
    return make_plugin_target(env, dir, os.path.basename(os.path.splitext(str(src[0]))[0]))

def make_plugin(env, dir, name, src):
    return env.Alias('plugins', env.SharedLibrary(make_plugin_target(env, dir, name), src))

def make_faust_plugin(env, dir, src):
    cpp = env.Faust(src)
    lib = env.SharedLibrary(make_faust_plugin_target(env, dir, cpp), cpp)
    name = os.path.basename(os.path.splitext(src)[0])
    svg = env.FaustSVG(os.path.join(dir, 'svg', name), src)
    xml = env.FaustXML(os.path.join(dir, 'xml', name), src)
    sc = env.FaustSC('skUG/Faust/Faust', xml)
    mod = env['FAUST2SC_HASKELL_MODULE']
    hs = env.FaustHaskell(os.path.join('haskell', '/'.join(mod.split('.'))), xml)
    return env.Alias('faust-plugins', [hs, lib, sc, svg, xml])

def make_faust_benchmark(env, src, prefix=''):
    target = os.path.splitext(str(src))[0]
    cpp = env.Faust(target + prefix + '.cpp', src)
    exe = env.Program(target, cpp)
    return env.Alias('faust-benchmarks', exe)

# Initialize plugin environment
pluginEnv = env.Copy(
	SHLIBPREFIX = '',
	SHLIBSUFFIX = env['PLUGIN_EXT'],
	FAUST_ARCHITECTURE = 'supercollider')
if env['PLATFORM'] == 'darwin':
    # build a MACH-O bundle
    pluginEnv['SHLINKFLAGS'] = '$LINKFLAGS -bundle -flat_namespace -undefined suppress'

# VEP
if not env['PLATFORM'] in ['windows']:
    vepEnv = pluginEnv.Copy()
    vepEnv.ParseConfig('pkg-config --cflags --libs fftw3f')
    make_plugin(
        vepEnv, 'skUG/VEP', 'VEPConvolution',
        [
         'src/VEP/VEPConv.cpp',
         'src/VEP/VEPFFT.cpp',
         'src/VEP/VEPPlugin.cpp'
         ])

# FM7
make_plugin(pluginEnv, 'skUG/FM7', 'FM7', ['src/FM7.cpp'])

# =====================================================================
# Faust plugins

# Faust environment
faustEnv = pluginEnv.Copy(
    FAUST_ARCHITECTURE = 'supercollider',
    FAUST2SC_HASKELL_MODULE = 'Sound.SC3.UGen.SKUG.Faust'
)
# Faust benchmark environment
faustBenchEnv = pluginEnv.Copy(
    FAUST_ARCHITECTURE = 'bench',
    PROGSUFFIX = '.bench'
)

FAUST_PLUGIN_SOURCE = Split('''
src/Faust/Blitz.dsp
src/Faust/Blitzaw.dsp
src/Faust/Blitzquare.dsp

src/Faust/ButterHP2.dsp
src/Faust/ButterHP4.dsp
src/Faust/ButterHP6.dsp

src/Faust/ButterHP2C.dsp
src/Faust/ButterHP4C.dsp
src/Faust/ButterHP6C.dsp

src/Faust/ButterLP2.dsp
src/Faust/ButterLP4.dsp
src/Faust/ButterLP6.dsp

src/Faust/ButterLP2C.dsp
src/Faust/ButterLP4C.dsp
src/Faust/ButterLP6C.dsp
''')

FAUST_BENCH_SOURCE = Split('''
src/Faust/benchmarks/mod.dsp
''')

for src in FAUST_PLUGIN_SOURCE:
    make_faust_plugin(faustEnv, 'skUG/Faust', src)
    make_faust_benchmark(faustBenchEnv, src, '_bench')

for src in FAUST_BENCH_SOURCE:
    make_faust_benchmark(faustBenchEnv, src)

env.Alias('plugins', 'faust-plugins')
env.Alias('benchmarks', 'faust-benchmarks')

Default('plugins')

# ======================================================================
# Cleanup
# ======================================================================

env.Clean('scrub',
          Split('config.log scache.conf .sconf_temp .sconsign.tmp .sconsign.dblite') +
          glob.glob('*.tbz2'))

# ======================================================================
# EOF
