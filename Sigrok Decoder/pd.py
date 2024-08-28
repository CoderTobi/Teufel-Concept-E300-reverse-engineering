##
## This file is made by me
##

# Erklärung zu meinen Bennenungen:
# Packet: 
# - Ein ADDRESS WRITE gefolgt von einem oder mehr DATA WRITE aka alles zwischen START und STOP
# - {"ss":3, "es":5, "address":0x46, "data":[0xE0, 0xD0]}
# Message: 
# - Besteht aus mehreren packages

import sigrokdecode as srd

class Decoder(srd.Decoder):
    api_version = 3
    id = 'subwoofer'
    name = 'Subwoofer'
    longname = 'Subwoofer Display Board Decoder'
    desc = 'Ein Decoder um die I2C Signale von meinem Subwoofer aufzubereiten'
    license = 'gplv2+'
    inputs = ['i2c']
    outputs = []
    tags = ['Teufel', 'Sub', 'Subwoofer']
    annotations = (
        ('test', 'Test'),
        ('packet_power', 'Power State'),
        ('packet_sep', 'separator'),
        ('packet_vol_all', 'Volume'),
        ('packet_vol_ch', 'Volume Chanel'),
        ('packet_err', 'Error'),
        ('info', 'Info'),
    )
    annotation_rows = (
        ('testrow', 'Test Row', (0,)),
        ('packets', 'Packets', (1,2,3,4,5)),
        ('sum', 'Summary', (6,)),
    )

    def __init__(self):
        self.reset()

    def reset(self):
        self.state = 'IDLE'
        self.counter = 0

    def start(self):
        self.out_ann = self.register(srd.OUTPUT_ANN)

    def put_an(self, ss, es, data):
        self.put(ss, es, self.out_ann, data)

    def processPacketBuffer(self):
        ss = self.packetBuffer["ss"]
        es = self.packetBuffer["es"]
        address = self.packetBuffer["address"]
        data = self.packetBuffer["data"]

        if data[0] == 0xF8:
            self.put_an(ss, es, [1, ['Power: ON','ON']])
        elif data[0] == 0xF9:
            self.put_an(ss, es, [1, ['Power: OFF','OFF']])
        elif data[0] == 0xC0:
            self.put_an(ss, es, [2, ['Separator', 'sep']])
        elif len(data) == 2:
            register, value = self.convertData(data)
            if address == 0x44:
                self.put_an(ss, es, [3, ['Volume-All: %i' % value]])
                self.ss_summary = ss
                self.volume = value
            elif address == 0x46:
                chanel = self.convertRegisterToChanel(register)
                self.put_an(ss, es, [4, ['Volume-Ch%i: %i' % (chanel,value)]])
                if chanel == 2:
                    self.put_an(self.ss_summary, es, [6, ['Volume: %i; Bass: %i;' % (self.volume,value)]])
        else:
            self.put_an(ss, es, [5, ['Error','Err']])

    def convertData(self, data):
        reg1 = (data[0] & 0xf0)
        reg2 = (data[1] & 0xf0)
        val1 = (data[0] & 0x0f)
        val2 = (data[1] & 0x0f)

        reg2 = (reg2 >> 4)
        val1 = val1 * 10

        reg = (reg1 | reg2)
        val = val1 + val2

        return reg, val

    def convertRegisterToChanel(self, register):
        if register == 0x45:
            return 1
        elif register == 0x89:
            return 2
        elif register == 0x01:
            return 3
        elif register == 0x23:
            return 4
        elif register == 0x67:
            return 5
        elif register == 0xAB:
            return 6
        

    def decode(self, ss, es, data):
        cmd, databyte = data

        # Debug
        #self.counter += 1
        #self.put_an(ss, es, [0, ['Counter %i | Cmd %s' % (self.counter, cmd) ]])

        # Wait for start of packet
        if self.state == "IDLE":
            if cmd == "START":
                # Start found
                self.packetBuffer = {"ss":ss, "data":[]}
                self.state = "READING"
                return
            else:
                # continue waiting
                return
        
        elif self.state == "READING":
            if cmd == "STOP":
                self.packetBuffer["es"] = es
                self.processPacketBuffer()
                self.state = "IDLE"
            elif cmd == "ADDRESS WRITE":
                self.packetBuffer["address"] = databyte
            elif cmd == "DATA WRITE":
                self.packetBuffer["data"].append(databyte)

            
