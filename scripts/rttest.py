#!/usr/bin/env python

import socket
import struct
import binascii
from datetime import datetime, timezone, timedelta

request = [
	0,      # 0,  uint32, Timstamp, integral
    0,      # 1,  uint32, Timestamp, fractional
	1,      # 2,  uint8 , Protocol version (current: 1) 
	0xCAFE, # 3,  uint32, Magic number for authorization (security) 
	0,      # 4,  uint32, Transversal spread in nanometers 
	0,      # 5,  uint32, Longitudinal spread in nanometers 
	0,      # 6,  int32 , Temperature in milli-degrees Celsius (signed) 
	0,      # 7,  uint32, Fluid flow in ml/min 
	0,      # 8,  uint32, Reserved field 
	0,      # 9,  uint8 , State control flags 
	0,      # 10, uint32, Rolling counter overall packets sent 
	0,      # 11, uint32, Packet CRC32 
]

prof_1 = [i for i in range(100)]
prof_2 = [100 for i in range(100)]
prof_3 = [100 - i * 2 for i in range(50)]

prof_spread = [i * (10**6)        for i in prof_1 + prof_2 + prof_3]
prof_temp   = [(i - 50) * (10**3) for i in prof_1 + prof_2 + prof_3]

sender = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
receiver = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
receiver.bind(('192.168.1.10', 50001))
dt = datetime.now(timezone.utc)

for s, t in zip(prof_spread, prof_temp):
    
    data, addr = receiver.recvfrom(1024)
    response = struct.unpack('>QBIIiIIBII', data)
    print('.', end='')
    
    if datetime.now(timezone.utc) - dt > timedelta(milliseconds=500):
        dt = datetime.now(timezone.utc)
        dt_cmd = dt + timedelta(milliseconds=250)
        ts_float = dt_cmd.timestamp()
        integr = int(ts_float)
        frac = int(abs(ts_float - integr) * (2**32))
        
        request[0] = integr
        request[1] = frac
        request[5] = s
        request[6] = t
        request[10] += 1

        body = struct.pack('>IIBIIIiIIBI', *request[:-1])
        crc = binascii.crc32(body) & 0xFFFFFFFF
        request[11] = crc
        
        msg = struct.pack('>IIBIIIiIIBII', *request)
        sender.sendto(msg, ('192.168.1.110', 50000))

        cur_s = response[3]
        cur_t = response[4]
        print('\n{}, request: {} nm, {} mDegC, last response: {} nm, {} mDegC'.format(dt, s, t, cur_s, cur_t))


