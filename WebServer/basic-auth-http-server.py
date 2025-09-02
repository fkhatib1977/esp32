from http.server import HTTPServer, SimpleHTTPRequestHandler
import base64
import json
import os

USERNAME = "johnny"
PASSWORD = "wrench"
AUTH_KEY = base64.b64encode(f"{USERNAME}:{PASSWORD}".encode()).decode()

class AuthHandler(SimpleHTTPRequestHandler):
    def is_authenticated(self):
        auth_header = self.headers.get("Authorization")
        return auth_header == f"Basic {AUTH_KEY}"

    def send_auth_challenge(self):
        self.send_response(401)
        self.send_header("WWW-Authenticate", 'Basic realm="Secure Area"')
        self.end_headers()
        self.wfile.write(b"Authentication required.")

    def do_POST(self):
        if not self.is_authenticated():
            self.send_auth_challenge()
            return

        if self.path == "/check":
            content_length = int(self.headers.get("Content-Length", 0))
            post_data = self.rfile.read(content_length)

            try:
                payload = json.loads(post_data.decode())
                print(f"[POST /check] Received payload: {payload}")
            except json.JSONDecodeError:
                self.send_response(400)
                self.end_headers()
                self.wfile.write(b"Invalid JSON")
                return

            if not os.path.exists("check.json"):
                self.send_response(404)
                self.end_headers()
                self.wfile.write(b"check.json not found")
                return

            with open("check.json", "r") as f:
                response_data = f.read()

            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.end_headers()
            self.wfile.write(response_data.encode())
        else:
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"Not Found")

    def do_GET(self):
        if not self.is_authenticated():
            self.send_auth_challenge()
            return

        if self.path == "/check":
            self.send_response(405)
            self.end_headers()
            self.wfile.write(b"Use POST for /check")
        else:
            super().do_GET()

if __name__ == "__main__":
    HTTPServer(("0.0.0.0", 8000), AuthHandler).serve_forever()