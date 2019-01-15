# vim: set filetype=python:

import os
import platform
import re
import subprocess
import locale

import lit.formats
import lit.util

from lit.llvm import llvm_config

# Configuration file for the 'lit' test runner.

# name: The name of this test suite.
config.name = 'allvm'

# testFormat: The test format to use to interpret tests.
config.test_format = lit.formats.ShTest(True) # not llvm_config.use_lit_shell)

# suffixes: A list of file extensions to treat as test files.
config.suffixes = ['.ll', '.c', '.cpp', '.test']

# excludes: A list of directories to exclude from the testsuite. The 'Inputs'
# subdirectories contain auxiliary inputs for various tests in their parent
# directories.
config.excludes = [ "Inputs", "source" ]

# test_source_root: The root path where tests are located.
config.test_source_root = os.path.dirname(__file__)

config.test_exec_root = os.path.join(config.allvm_obj_root, 'test')

llvm_config.use_default_substitutions()
# llvm_config.use_lld()

#tool_patterns = [
#    'llc', 'llvm-as', 'llvm-mc', 'llvm-nm', 'llvm-objdump', 'llvm-pdbutil',
#    'llvm-dwarfdump', 'llvm-readelf', 'llvm-readobj', 'obj2yaml', 'yaml2obj',
#    'opt', 'llvm-dis']
#
# llvm_config.add_tool_substitutions(tool_patterns)

# Propagate some variables from the host environment.
#llvm_config.with_system_environment(
#    ['HOME', 'INCLUDE', 'LIB', 'TMP', 'TEMP', 'ASAN_SYMBOLIZER_PATH', 'MSAN_SYMBOLIZER_PATH'])

#newpath = config.allvm_tools_dir "@LLVM_RUNTIME_OUTPUT_INTDIR@:@LLVM_TOOLS_BINARY_DIR@"
#curpath = os.environ.get('PATH', '')
#if curpath:
#    newpath += ":" + curpath
#config.environment['PATH'] = newpath
config.environment['XDG_CACHE_HOME'] = config.test_exec_root + "/cache";

# TODO: Get this more reliably/directly: " = @LIBNONE_PATH@" or something
libnone = "@LLVM_LIBRARY_OUTPUT_INTDIR@/libnone.a"

config.substitutions.append( ('%libnone', libnone) )
config.substitutions.append( ('%alley', "@LLVM_RUNTIME_OUTPUT_INTDIR@/alley") )

#libpath = os.environ.get('LD_LIBRARY_PATH', '')
#if libpath:
#    libpath = ":" + libpath
#config.environment['LD_LIBRARY_PATH'] = "@OBJDIR@/prefix/lib" + libpath

# When running under valgrind, we mangle '-vg' onto the end of the triple so we
# can check it with XFAIL and XTARGET.
#if lit_config.useValgrind:
#    config.target_triple += '-vg'

# Running on ELF based *nix
#if platform.system() in ['FreeBSD', 'Linux']:
#    config.available_features.add('system-linker-elf')

# Set if host-cxxabi's demangler can handle target's symbols.
#if platform.system() not in ['Windows']:
#    config.available_features.add('demangler')

#llvm_config.feature_config(
#    [('--build-mode', {'DEBUG': 'debug'}),
#     ('--assertion-mode', {'ON': 'asserts'}),
#     ('--targets-built', {'AArch64': 'aarch64',
#                          'AMDGPU': 'amdgpu',
#                          'ARM': 'arm',
#                          'AVR': 'avr',
#                          'Hexagon': 'hexagon',
#                          'Mips': 'mips',
#                          'PowerPC': 'ppc',
#                          'Sparc': 'sparc',
#                          'WebAssembly': 'wasm',
#                          'X86': 'x86'})
#     ])

# Set a fake constant version so that we get consistent output.
config.environment['LLD_VERSION'] = 'LLD 1.0'
config.environment['LLD_IN_TEST'] = '1'

tar_executable = lit.util.which('tar', config.environment['PATH'])
if tar_executable:
    tar_version = subprocess.Popen(
        [tar_executable, '--version'], stdout=subprocess.PIPE, env={'LANG': 'C'})
    if 'GNU tar' in tar_version.stdout.read().decode():
        config.available_features.add('gnutar')
    tar_version.wait()
