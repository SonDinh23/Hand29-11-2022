import ssl
from http.server import HTTPServer, SimpleHTTPRequestHandler

class MyRequestHandler(SimpleHTTPRequestHandler):
  protocol_version = "HTTP/1.1"

server_address = ('', 8000)
# server = HTTPServer(server_address, MyRequestHandler)


httpd = HTTPServer(server_address, SimpleHTTPRequestHandler)
httpd.socket = ssl.wrap_socket(httpd.socket,
                               server_side=True,
                               certfile='localhost.pem',
                               keyfile='localhost-key.pem',
                               ssl_version=ssl.PROTOCOL_TLS)



httpd.serve_forever()
