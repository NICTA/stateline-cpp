import socket
import struct
import subprocess
import sys
import unittest

class ProtocolTests(unittest.TestCase):
    WORKER_PATH = ""
    SERVER_PORT = "5000"

    def setUp(self):
        # Start the server
        sock = socket.socket()
        sock.bind(("0.0.0.0", 5000))
        sock.listen(1)

        # Start the worker
        self.worker = subprocess.Popen([self.WORKER_PATH, "localhost:" + self.SERVER_PORT])

        # Accept the connection
        self.conn, _ = sock.accept()


    def test_hello(self):
        data = self.conn.recv(1024)
        self.assertEqual(1 + 1 + 4 + 4, len(data))

        msg_type, version, job_type_from, job_type_to = struct.unpack('=bbII', data)

        self.assertEqual(1, msg_type)
        self.assertEqual(0, version)
        self.assertEqual(0, job_type_from)
        self.assertEqual(0, job_type_to)


    def tearDown(self):
        self.worker.kill()


if __name__ == '__main__':
    # TODO: this is kind of hacky...use py.test with parameterized tests?
    if len(sys.argv) > 1:
        ProtocolTests.WORKER_PATH = sys.argv.pop()

    unittest.main()
