##
## This file is made by me
##

import sigrokdecode as srd

class Decoder(srd.Decoder):
    api_version = 3
    id = 'test'
    name = 'Test'
    longname = 'Test Decoder'
    desc = 'Ein Decoder zum rum testen'
    license = 'gplv2+'
    inputs = ['i2c']
    outputs = []
    tags = ['Test']
    annotations = (
        ('test', 'Test'),
        ('test2', 'Test2'),
    )
    annotation_rows = (
        ('testrow', 'Test Row', (0,)),
        ('blocks', 'Blocks', (1,)),
    )

    def __init__(self):
        self.reset()

    def reset(self):
        self.state = 'IDLE'
        self.counter = 0

    def start(self):
        self.out_ann = self.register(srd.OUTPUT_ANN)

    def putx(self, data):
        self.put(self.ss, self.es, self.out_ann, data)

    def decode(self, ss, es, data):
        cmd, databyte = data
        self.ss, self.es = ss, es
        self.counter += 1
        self.putx([0, ['Counter %i | Cmd %s' % (self.counter, cmd) ]])

        if cmd == 'DATA WRITE':
            self.putx([1, ['Byte: %x' % databyte ]])
            pass

        
