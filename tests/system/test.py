import socket
import struct
import subprocess
import sys
import time
import unittest

class ProtocolTests(unittest.TestCase):
    WORKER_PATH = ""
    SERVER_PORT = "5000"

    @classmethod
    def setUpClass(cls):
        sock = socket.socket()
        sock.bind(("0.0.0.0", 5000))
        sock.listen(1)

        cls.sock = sock

    @classmethod
    def tearDownClass(cls):
        cls.sock.close()

    def setUp(self):
        # Start the worker
        self.worker = subprocess.Popen([self.WORKER_PATH, "localhost:" + self.SERVER_PORT])

        # Accept the connection
        self.conn, _ = self.sock.accept()

    def _assert_hello(self):
        data = self.conn.recv(1024)
        self.assertEqual(1 + 1 + 4 + 4, len(data))

        msg_type, version, job_type_from, job_type_to = struct.unpack('=bbII', data)

        self.assertEqual(1, msg_type)
        self.assertEqual(0, version)
        self.assertEqual(0, job_type_from)
        self.assertEqual(0, job_type_to)


    def _send_job(self, job_id, job_type, vector):
        msg = struct.pack('=bII{}d'.format(len(vector)), 2, job_id, job_type, *vector)
        self.conn.send(msg)


    def _assert_result(self, expected_job_id, expected_result):
        data = self.conn.recv(1024)
        self.assertEqual(1 + 4 + 8, len(data))

        msg_type, job_id, result = struct.unpack('=bId', data)

        self.assertEqual(3, msg_type)
        self.assertEqual(expected_job_id, job_id)
        self.assertEqual(expected_result, result)


    def test_hello(self):
        self._assert_hello()


    def test_req_rep(self):
        self._assert_hello()
        self._send_job(1, 1, [1, 2, 3])
        self._assert_result(1, 6)


    def tearDown(self):
        self.worker.kill()

        # Give it time to die
        time.sleep(1)


if __name__ == '__main__':
    # TODO: this is kind of hacky...use py.test with parameterized tests?
    if len(sys.argv) > 1:
        ProtocolTests.WORKER_PATH = sys.argv.pop()

    unittest.main()
