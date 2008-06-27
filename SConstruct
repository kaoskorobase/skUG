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
# setup
# ======================================================================

EnsureSConsVersion(0, 96)
EnsurePythonVersion(2, 3)
SConsignFile()

# ======================================================================
# constants
# ======================================================================

PACKAGE = 'vep'
VERSION = '0.1'

# ======================================================================
# util
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
# command line options
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
    PathOption('SC_SOURCE_DIR',
               'SuperCollider source directory', '../supercollider/'),
    BoolOption('SSE',
               'Build with SSE support', 1),
    ('CROSSCOMPILE', 'Platform to crosscompile for', 'None')
    )

# ======================================================================
# basic environment
# ======================================================================

env = Environment(options = opts,
                  PACKAGE = PACKAGE,
                  VERSION = VERSION,
                  ENV = make_os_env('PATH', 'PKG_CONFIG_PATH'))

# set platform
if env['CROSSCOMPILE'] != 'None':
    if env['CROSSCOMPILE'] == 'mingw':
        set_platform(env, 'windows', 'i386')
        env.Tool('crossmingw')
    else:
        print 'Unknown cross compilation environment: %s' % env['CROSSCOMPILE']
        Exit(1)
else:
    set_platform(env, os.uname()[0].lower(), os.uname()[4].lower())

env.Append(
    CCFLAGS = env['CUSTOMCCFLAGS'],
    CXXFLAGS = env['CUSTOMCXXFLAGS'])

# defines and compiler flags
env.Append(
    CPPDEFINES = ['_REENTRANT', env['PLATFORM_SYMBOL']],
    CCFLAGS = ['-Wno-unknown-pragmas'],
    CXXFLAGS = ['-Wno-deprecated']
    )

# benchmarking
if env['BENCHMARK']:
    env.Append(CPPDEFINES = ['VEP_BENCHMARK'])

# debugging flags
if env['DEBUG']:
    env.Append(CCFLAGS = '-g')
else:
    env.Append(CPPDEFINES = ['NDEBUG'])

# platform specific
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

# finish
opts.Save('scache.conf', env)
Help(opts.GenerateHelpText(env))

# ======================================================================
# plugins
# ======================================================================

def make_plugin_target(env, dir, name):
    if env['PLATFORM'] == 'darwin':
        pdir = 'osx'
    elif env['PLATFORM'] == 'linux':
        pdir = 'linux'
    elif env['PLATFORM'] == 'windows':
        pdir = 'windows'
    else
        pdir = env['PLATFORM']
    return os.path.join(dir, pdir, name)

pluginEnv = env.Copy(
	SHLIBPREFIX = '', SHLIBSUFFIX = env['PLUGIN_EXT']
	)
pluginEnv.Append(
    CPPPATH = map(lambda f: os.path.join(env['SC_SOURCE_DIR'], 'headers', f),
                  ['common', 'plugin_interface', 'server']))
if env['PLATFORM'] == 'darwin':
    # build a MACH-O bundle
    pluginEnv['SHLINKFLAGS'] = '$LINKFLAGS -bundle -flat_namespace -undefined suppress'
plugins = []

# VEP
if not env['PLATFORM'] in ['windows']:
    vepEnv = pluginEnv.Copy()
    vepEnv.ParseConfig('pkg-config --cflags --libs fftw3f')
    plugins.append(
        vepEnv.SharedLibrary(
        'skUG/VEP/VEPConvolution',
        [
         'src/VEP/VEPConv.cpp',
         'src/VEP/VEPFFT.cpp',
         'src/VEP/VEPPlugin.cpp'
         ]))

# FM7
plugins.append(
    pluginEnv.SharedLibrary(make_plugin_target(env, 'skUG/FM7', 'FM7'), ['src/FM7.cpp']))

env.Alias('plugins', plugins)
Default('plugins')

# ======================================================================
# cleanup
# ======================================================================

env.Clean('scrub',
          Split('config.log scache.conf .sconf_temp .sconsign.tmp .sconsign.dblite') +
          glob.glob('*.tbz2'))

# ======================================================================
# EOF
