import sys
env = Environment()
env = env.Clone()


if not env.GetOption('clean'):
    print 'CC: ', env['CC']
    print 'CXX: ', env['CXX']
    print 'CFLAGS: ', env.get('CFLAGS', "")
    print 'CXXFLAGS: ', env.get('CXXFLAGS', "")
    print 'CCFLAGS: ', env.get('CCFLAGS', "")
    print 'CPPFLAGS: ', env.get('CPPFLAGS', "")
    print 'Platform: ', env['PLATFORM']

    conf = Configure(env)
    conf.CheckCC()
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

env.Append(CXXFLAGS = ' -Wall -g -O2')
env.Append(CPPFLAGS = ' -DNDEBUG')
env.Append(LINKFLAGS = ' ')
SOURCE = [
    'src/city.cc',
    'src/document.cc',
    'src/index.cc',
    'src/main.cc',
    'src/wand.cc'
]
env.Program('wand-test', SOURCE)
