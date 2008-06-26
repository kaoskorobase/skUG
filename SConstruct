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

PLATFORM = os.uname()[0].lower()
CPU = os.uname()[4].lower()

if PLATFORM == 'darwin':
    PLATFORM_SYMBOL = 'SC_DARWIN'
    PLUGIN_EXT = '.scx'
    DEFAULT_PREFIX = '/'
elif PLATFORM == 'linux':
    PLATFORM_SYMBOL = 'SC_LINUX'
    PLUGIN_EXT = '.so'
    DEFAULT_PREFIX = '/usr/local'
# elif PLATFORM == 'windows':
#     PLATFORM_SYMBOL = 'SC_WIN32'
#     PLUGIN_EXT = '.scx'
#     DEFAULT_PREFIX = '/'
else:
    print 'Unknown platform: %s' % PLATFORM
    Exit(1)

# ======================================================================
# util
# ======================================================================

def flatten_dir(dir):
    res = []
    for root, dirs, files in os.walk(dir):
        if 'CVS' in dirs: dirs.remove('CVS')
        for f in files:
            res.append(os.path.join(root, f))
    return res

def install_dir(env, src_dir, dst_dir, filter_re, strip_levels=0):
    nodes = []
    for root, dirs, files in os.walk(src_dir):
        src_paths = []
        dst_paths = []
        if '.svn' in dirs: dirs.remove('.svn')
        for d in dirs[:]:
            if filter_re.match(d):
                src_paths += flatten_dir(os.path.join(root, d))
                dirs.remove(d)
        for f in files:
            if filter_re.match(f):
                src_paths.append(os.path.join(root, f))
        dst_paths += map(
            lambda f:
            os.path.join(
            dst_dir,
            *f.split(os.path.sep)[strip_levels:]),
            src_paths)
        nodes += env.InstallAs(dst_paths, src_paths)
    return nodes

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
    PathOption('DESTDIR',
               'Intermediate installation prefix for packaging', '/'),
    PathOption('PREFIX',
               'Installation prefix', DEFAULT_PREFIX),
    PathOption('SCSRCDIR',
               'SuperCollider source directory', '../SuperCollider3/'),
    BoolOption('SSE',
               'Build with SSE support', 1),
    )

# ======================================================================
# basic environment
# ======================================================================

env = Environment(options = opts,
                  PACKAGE = PACKAGE,
                  VERSION = VERSION,
                  ENV = make_os_env('PATH', 'PKG_CONFIG_PATH'))
env.Append(
    CCFLAGS = env['CUSTOMCCFLAGS'],
    CXXFLAGS = env['CUSTOMCXXFLAGS'])

# defines and compiler flags
env.Append(
    CPPDEFINES = ['_REENTRANT', PLATFORM_SYMBOL],
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
if CPU == 'ppc':
    env.Append(CCFLAGS = '-fsigned-char')

if env['ALTIVEC'] and (CPU == 'ppc'):
    altiEnv = env.Copy()
    altiEnv.Append(CCFLAGS = ['-maltivec'])
    altiConf = Configure(altiEnv)
    has_vec = altiConf.CheckCHeader('altivec.h')
    altiConf.Finish()
    if has_vec:
        env.Append(CCFLAGS = ['-maltivec', '-mabi=altivec'])
elif env['SSE'] and re.match("^i?[0-9x]86", CPU):
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

pluginEnv = env.Copy(
	SHLIBPREFIX = '', SHLIBSUFFIX = PLUGIN_EXT
	)
pluginEnv.Append(
    CPPPATH = map(lambda f: os.path.join(env['SCSRCDIR'], 'headers', f),
                  ['common', 'plugin_interface', 'server']))
if PLATFORM == 'darwin':
    # build a MACH-O bundle
    pluginEnv['SHLINKFLAGS'] = '$LINKFLAGS -bundle -flat_namespace -undefined suppress'
plugins = []

# VEP

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
    pluginEnv.SharedLibrary('skUG/FM7/FM7', ['src/FM7.cpp']))

env.Alias('plugins', plugins)
Default('plugins')

# ======================================================================
# distribution
# ======================================================================

DIST_FILES = Split('''
lib/VEP.sc
SConstruct
src/VEP.cpp
''')

DIST_DIRS = Split('''
''')

def dist_paths():
    paths = DIST_FILES[:]
    for base in DIST_DIRS:
        for root, dirs, files in os.walk(base):
            if '.svn' in dirs: dirs.remove('.svn')
            for path in files:
                paths.append(os.path.join(root, path))
    paths.sort()
    return paths

def build_tar(env, target, source):
    paths = dist_paths()
    tarfile_name = str(target[0])
    tmp, ext = os.path.splitext(tarfile_name)
    tar_name = os.path.splitext(os.path.basename(tmp))[0]
    tar = tarfile.open(tarfile_name, "w:" + ext[1:])
    for path in paths:
        tar.add(path, os.path.join(tar_name, path))
    tar.close()

snapshot_tarball = PACKAGE + '-' + time.strftime("%Y%m%d", time.localtime()) + '.tar.bz2'
env.Alias('snapshot-dist', snapshot_tarball)
env.AlwaysBuild(env.Command(snapshot_tarball, None, build_tar))

release_tarball = PACKAGE + '-' + VERSION + '.tar.bz2'
env.Alias('release-dist', release_tarball)
env.AlwaysBuild(env.Command(release_tarball, None, build_tar))

env.Alias('dist', ['snapshot-dist'])

# ======================================================================
# cleanup
# ======================================================================

env.Clean('scrub',
          Split('config.log scache.conf .sconf_temp .sconsign.tmp .sconsign.dblite') +
          glob.glob('*.tbz2'))

# ======================================================================
# EOF
