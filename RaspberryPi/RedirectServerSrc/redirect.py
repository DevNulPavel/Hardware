#! /usr/bin/env python2

import httplib
import SimpleHTTPServer
import SocketServer

#PORT = 8080
#URL = "devnulpavel.ddns.net"
PORT = 8888
URL = '192.168.1.3'
PATH = '/snapshot.cgi?loginuse=DevNul1&loginpas=Azsxdcfv1234'

class FakeRedirect(SimpleHTTPServer.SimpleHTTPRequestHandler):
    def do_GET(self):		
		conn = httplib.HTTPConnection(URL, PORT, timeout=5)
		conn.request("GET", PATH)
		response = conn.getresponse()
		status = response.status
		if status == 200:
			contentType = response.getheader("Content-type")
			data = response.read()

			self.send_response(status)
			self.send_header("Content-type", contentType)
			self.end_headers()
			self.wfile.write(data)
			self.wfile.close()
		else:
			self.wfile.close()

SocketServer.TCPServer(("", 8080), FakeRedirect).serve_forever()