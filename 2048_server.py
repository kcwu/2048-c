import SocketServer
import BaseHTTPServer
import datetime
import re
import hashlib
import json
import os
import subprocess
import base64

PORT = 8765
config_fn = 'config.txt'

try:
    os.mkdir('tmp')
except OSError:
    pass

def md5(s):
    m = hashlib.md5()
    m.update(s)
    return m.hexdigest()

class MyHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def handle(self):
        ip, port =  self.client_address
        #print ip
        if ip in (
                '172.30.210.194', # kcwu-z620.tpe
                '127.0.0.1',
                ):
            BaseHTTPServer.BaseHTTPRequestHandler.handle(self)
        else:
            self.raw_requestline = self.rfile.readline()
            if self.parse_request():
                self.send_error(403)

    def do_GET(self):
        #print self.path
        if self.path == '/config':
            for line in file(config_fn):
                m = re.match(r'^(\d+)-(\d+)\s+(\d+)$', line)
                assert m
                s, t, c = map(int, m.group(1, 2, 3))
                if s <= datetime.datetime.now().hour < t:
                    self.wfile.write(str(c))
            return

        if self.path == '/killall':
            subprocess.check_output(['killall', '-r', '2048.*'])
            return

    def do_POST(self):
        #print self.path
        if self.path == '/run':
            length = int(self.headers.getheader('content-length'))
            exe, arg = json.loads(self.rfile.read(length))
            exe = base64.b64decode(exe)
            fn = 'tmp/2048.%s' % md5(exe)
            if not os.path.exists(fn):
                fn_tmp = fn + '.tmp.%d' % os.getpid()
                with file(fn_tmp, 'wb') as fp:
                    fp.write(exe)
                os.chmod(fn_tmp, 0700)
                os.rename(fn_tmp, fn)

            cmd = [fn] + arg
            print cmd

            try:
                output = subprocess.check_output(cmd)
                result = dict(done='ok', output=output)
            except Exception, e:
                result = dict(done='error', msg=str(e))
            self.wfile.write(json.dumps(result))


SocketServer.ForkingTCPServer.allow_reuse_address = True
httpd = SocketServer.ForkingTCPServer(("", PORT), MyHandler)

print "serving at port", PORT
httpd.serve_forever()

