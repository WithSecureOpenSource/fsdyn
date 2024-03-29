import os
import fsenv

DIRECTORIES = [
    'src',
    'test',
    'components/avltree' ]

TARGET_DEFINES = {
    'freebsd_amd64': [],
    'linux32': ['_FILE_OFFSET_BITS=64'],
    'linux64': [],
    'linux_arm64': [],
    'openbsd_amd64': [],
    'darwin': []
}

TARGET_FLAGS = {
    'freebsd_amd64': '',
    'linux32': '-m32 ',
    'linux64': '',
    'linux_arm64': '',
    'openbsd_amd64': '',
    'darwin': '-mmacosx-version-min=10.13 '
}

def construct():
    ccflags = (
        ' -g -O2 -Wall -Werror '
        '-Wno-parentheses '
    )
    prefix = ARGUMENTS.get('prefix', '/usr/local')
    arch_env = {}
    for target_arch in fsenv.target_architectures():
        arch_env[target_arch] = env = Environment(
            NAME="fsdyn",
            FSNAME="avltree",
            ARCH=target_arch,
            PREFIX=prefix,
            CCFLAGS=TARGET_FLAGS[target_arch] + ccflags,
            CPPDEFINES=TARGET_DEFINES[target_arch],
            LINKFLAGS=TARGET_FLAGS[target_arch],
            tools=['default', 'textfile', 'fscomp', 'scons_compilation_db'])
        fsenv.consider_environment_variables(env)
    for target_arch in fsenv.target_architectures():
        build_dir = os.path.join(
            fsenv.STAGE,
            target_arch,
            ARGUMENTS.get('builddir', 'build'))
        arch_env[target_arch].CompilationDB(
            os.path.join(build_dir, "compile_commands.json"))
        for directory in DIRECTORIES:
            env = arch_env[target_arch].Clone()
            env.SetCompilationDB(arch_env[target_arch].GetCompilationDB())
            host_env = arch_env[fsenv.HOST_ARCH].Clone()
            SConscript(dirs=directory,
                       exports=['env', 'host_env'],
                       duplicate=False,
                       variant_dir=os.path.join(build_dir, directory))
        Clean('.', build_dir)

if __name__ == 'SCons.Script':
    construct()
