import sys

env = Environment()
env = env.Clone()
conf = Configure(env)
conf.CheckCXX()

unordered_map = False

if conf.CheckHeader('unordered_map', language='c++'):
    unordered_map = True

if conf.CheckHeader('tr1/unordered_map', language='c++'):
    unordered_map = True
    env.Append(CPPFLAGS = ' -DHAVE_STD_TR1_UNORDERED_MAP')

if not unordered_map:
    print "no <unordered_map> or <tr1/unordered_map> found"
    sys.exit(1)

env.Append(CCFLAGS = '-Wall -g')

SOURCE = [
    'src/city.cc',
    'src/document.cc',
    'src/index.cc',
    'src/main.cc',
    'src/wand.cc'
]
env.Program('wand-test', SOURCE)
