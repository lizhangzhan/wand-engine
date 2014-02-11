#!  /usr/bin/env python
# -*- coding:utf-8 -*-
import sys
from xml.sax import parse, handler, SAXException

CAP_FEATURES = 'cap_features'

class CapFeaturesHandler(handler.ContentHandler):

    def __init__(self):
        self.enter = False
        self.name = ''
        self.weight = ''

    def startDocument(self):
        pass

    def endDocument(self):
        pass

    def startElement(self, name, attrs):
        if self.enter:
            if attrs.has_key('weight'):
                self.weight = attrs.get('weight')
        else:
            if name == CAP_FEATURES:
                self.enter = True
                print CAP_FEATURES

    def endElement(self, name):
        if self.enter:
            if name == CAP_FEATURES:
                self.enter = False
        else:
            pass

    def characters(self, content):
        if content.isspace():
            return
        if self.enter:
            self.name = content
            print '    %s %s' % (self.name, self.weight)


if __name__ == '__main__':
    for file in sys.argv[1:]:
        parse(file, CapFeaturesHandler())
