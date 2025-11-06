import os,sys

appname = sys.argv[1]

extenders = ['h', 'c', 'cdl','cfg']

for extender in extenders:
    f = open('template/template.'+extender, 'r', encoding='utf-8')
    s = f.read().replace('template.h', appname+'.h')
    f.close()

    f = open(appname + '/' + appname + '.' + extender, 'w',encoding='utf-8')
    s = f.write(s)
    f.close()

f = open('template/Makefile.inc', 'r', encoding='utf-8')
s = f.read()
f.close()

f = open(appname + '/Makefile.inc', 'w', encoding='utf-8')
s = f.write(s)
f.close()
