#!/usr/bin/env python

"""
pyboard interface

This module provides the Pyboard class, used to communicate with and
control the pyboard over a serial USB connection.

Example usage:

    import pyboard
    pyb = pyboard.Pyboard('/dev/ttyACM0')

Or:

    pyb = pyboard.Pyboard('192.168.1.1')

Then:

    pyb.enter_raw_repl()
    pyb.exec('pyb.LED(1).on()')
    pyb.exit_raw_repl()

Note: if using Python2 then pyb.exec must be written as pyb.exec_.
To run a script from the local machine on the board and print out the results:

    import pyboard
    pyboard.execfile('test.py', device='/dev/ttyACM0')

This script can also be run directly.  To execute a local script, use:

    ./pyboard.py test.py

Or:

    python pyboard.py test.py

"""

import sys
import time

try:
    stdout = sys.stdout.buffer
except AttributeError:
    # Python2 doesn't have buffer attr
    stdout = sys.stdout

def stdout_write_bytes(b):
    b = b.replace(b"\x04", b"")
    stdout.write(b)
    stdout.flush()

class PyboardError(BaseException):
    pass

class TelnetToSerial:
    def __init__(self, ip, user, password, read_timeout=None):
        import telnetlib
        self.tn = telnetlib.Telnet(ip, timeout=15)
        self.read_timeout = read_timeout
        if b'Login as:' in self.tn.read_until(b'Login as:', timeout=read_timeout):
            self.tn.write(bytes(user, 'ascii') + b"\r\n")

            if b'Password:' in self.tn.read_until(b'Password:', timeout=read_timeout):
                # needed because of internal implementation details of the telnet server
                time.sleep(0.2)
                self.tn.write(bytes(password, 'ascii') + b"\r\n")

                if b'for more information.' in self.tn.read_until(b'Type "help()" for more information.', timeout=read_timeout):
                    # login succesful
                    from collections import deque
                    self.fifo = deque()
                    return

        raise PyboardError('Failed to establish a telnet connection with the board')

    def __del__(self):
        self.close()

    def close(self):
        try:
            self.tn.close()
        except:
            # the telnet object might not exist yet, so ignore this one
            pass

    def read(self, size=1):
        while len(self.fifo) < size:
            timeout_count = 0
            data = self.tn.read_eager()
            if len(data):
                self.fifo.extend(data)
                timeout_count = 0
            else:
                time.sleep(0.25)
                if self.read_timeout is not None and timeout_count > 4 * self.read_timeout:
                    break
                timeout_count += 1

        data = b''
        while len(data) < size and len(self.fifo) > 0:
            data += bytes([self.fifo.popleft()])
        return data

    def write(self, data):
        self.tn.write(data)
        return len(data)

    def inWaiting(self):
        n_waiting = len(self.fifo)
        if not n_waiting:
            data = self.tn.read_eager()
            self.fifo.extend(data)
            return len(data)
        else:
            return n_waiting

plat = sys.platform.lower()
if plat[:5] == 'linux':
    import os

    class SerialFileLock:
        LOCK_DIR = '/var/lock'

        def __init__(self, dev_name):
            self._dev_name = dev_name
            self._lockname = self.lockname()

        def lockname(self):
            os.stat(SerialFileLock.LOCK_DIR)   # throws an exception if the directory doesn't exist
            dev_name = self._dev_name
            if dev_name.startswith('/dev/'):
                dev_name = dev_name[5:]
            return '{}/LCK..{}'.format(SerialFileLock.LOCK_DIR, dev_name.replace('/', '_'))

        def lock(self):
            try:
                with open(self._lockname, 'rb') as file:
                    data = file.read(16)
                    if len(data) == 4:
                        # old-style - pid is binary
                        pid = struct.unpack('@i', data)[0]
                    else:
                        try:
                            pid = int(data)
                        except:
                            pid = 0
                    if pid > 0:
                        try:
                            os.kill(pid, 0)
                            self._lockname = None
                            raise PyboardError("{} is already open by pid {}".format(self._dev_name, pid))
                        except OSError:
                            # Process doesn't exist - stale lock file
                            print('Removing stale lock:', self._lockname)
                            time.sleep(1)
                            os.unlink(self._lockname)
            except IOError:
                # This is the normal path - the lockfile doesn't exist
                pass
            with open(self._lockname, 'w') as file:
                file.write('{}\n'.format(os.getpid()))

        def unlock(self):
            if self._lockname:
                os.unlink(self._lockname)

else:
    class SerialFileLock:
        def __init__(self, dev_name):
            pass
        def lock(self):
            pass
        def unlock(self):
            pass

if plat[:6] == 'darwin':
    import fcntl

    class SerialFlock:

        def __init__(self, file, dev_name):
            self.file = file
            self.dev_name = dev_name

        def lock(self):
            try:
                fcntl.flock(self.file.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
            except IOError:
                raise PyboardError("{} is already open".format(self.dev_name))

        def unlock(self):
            fcntl.flock(self.file.fileno(), fcntl.LOCK_UN)
else:
    class SerialFlock:
        def __init__(self, file, dev_name):
            pass
        def lock(self):
            pass
        def unlock(self):
            pass


class Pyboard:
    def __init__(self, device, baudrate=115200, user='micro', password='python', wait=0):
        if device and device[0].isdigit() and device[-1].isdigit() and device.count('.') == 3:
            # device looks like an IP address
            self.serial = TelnetToSerial(device, user, password, read_timeout=10)
        else:
            import serial
            self.file_lock = SerialFileLock(device)
            self.file_lock.lock()
            delayed = False
            for attempt in range(wait + 1):
                try:
                    self.serial = serial.Serial(device, baudrate=baudrate, interCharTimeout=1)
                    break
                except (OSError, IOError): # Py2 and Py3 have different errors
                    if wait == 0:
                        continue
                    if attempt == 0:
                        sys.stdout.write('Waiting {} seconds for pyboard '.format(wait))
                        delayed = True
                time.sleep(1)
                sys.stdout.write('.')
                sys.stdout.flush()
            else:
                if delayed:
                    print('')
                raise PyboardError('failed to access ' + device)
            if delayed:
                print('')
            self.flock = SerialFlock(self.serial, device)
            self.flock.lock()

    def close(self):
        self.flock.unlock()
        self.serial.close()
        self.file_lock.unlock()

    def read_until(self, min_num_bytes, ending, timeout=10, data_consumer=None):
        data = self.serial.read(min_num_bytes)
        if data_consumer:
            data_consumer(data)
        timeout_count = 0
        while True:
            if data.endswith(ending):
                break
            elif self.serial.inWaiting() > 0:
                new_data = self.serial.read(1)
                data = data + new_data
                if data_consumer:
                    data_consumer(new_data)
                timeout_count = 0
            else:
                timeout_count += 1
                if timeout is not None and timeout_count >= 100 * timeout:
                    break
                time.sleep(0.01)
        return data

    def enter_raw_repl(self):
        self.serial.write(b'\r\x03\x03') # ctrl-C twice: interrupt any running program

        # flush input (without relying on serial.flushInput())
        n = self.serial.inWaiting()
        while n > 0:
            self.serial.read(n)
            n = self.serial.inWaiting()

        self.serial.write(b'\r\x01') # ctrl-A: enter raw REPL
        data = self.read_until(1, b'raw REPL; CTRL-B to exit\r\n>')
        if not data.endswith(b'raw REPL; CTRL-B to exit\r\n>'):
            print(data)
            raise PyboardError('could not enter raw repl')

        self.serial.write(b'\x04') # ctrl-D: soft reset
        data = self.read_until(1, b'soft reboot\r\n')
        if not data.endswith(b'soft reboot\r\n'):
            print(data)
            raise PyboardError('could not enter raw repl')
        # By splitting this into 2 reads, it allows boot.py to print stuff,
        # which will show up after the soft reboot and before the raw REPL.
        data = self.read_until(1, b'raw REPL; CTRL-B to exit\r\n')
        if not data.endswith(b'raw REPL; CTRL-B to exit\r\n'):
            print(data)
            raise PyboardError('could not enter raw repl')

    def exit_raw_repl(self):
        self.serial.write(b'\r\x02') # ctrl-B: enter friendly REPL

    def follow(self, timeout, data_consumer=None):
        # wait for normal output
        data = self.read_until(1, b'\x04', timeout=timeout, data_consumer=data_consumer)
        if not data.endswith(b'\x04'):
            raise PyboardError('timeout waiting for first EOF reception')
        data = data[:-1]

        # wait for error output
        data_err = self.read_until(1, b'\x04', timeout=timeout)
        if not data_err.endswith(b'\x04'):
            raise PyboardError('timeout waiting for second EOF reception')
        data_err = data_err[:-1]

        # return normal and error output
        return data, data_err

    def exec_raw_no_follow(self, command):
        if isinstance(command, bytes):
            command_bytes = command
        else:
            command_bytes = bytes(command, encoding='utf8')

        # check we have a prompt
        data = self.read_until(1, b'>')
        if not data.endswith(b'>'):
            raise PyboardError('could not enter raw repl')

        # write command
        for i in range(0, len(command_bytes), 256):
            self.serial.write(command_bytes[i:min(i + 256, len(command_bytes))])
            time.sleep(0.01)
        self.serial.write(b'\x04')

        # check if we could exec command
        data = self.serial.read(2)
        if data != b'OK':
            raise PyboardError('could not exec command')

    def exec_raw(self, command, timeout=10, data_consumer=None):
        self.exec_raw_no_follow(command);
        return self.follow(timeout, data_consumer)

    def eval(self, expression):
        ret = self.exec_('print({})'.format(expression))
        ret = ret.strip()
        return ret

    def exec_(self, command):
        ret, ret_err = self.exec_raw(command)
        if ret_err:
            raise PyboardError('exception', ret, ret_err)
        return ret

    def execfile(self, filename):
        with open(filename, 'rb') as f:
            pyfile = f.read()
        return self.exec_(pyfile)

    def get_time(self):
        t = str(self.eval('pyb.RTC().datetime()'), encoding='utf8')[1:-1].split(', ')
        return int(t[4]) * 3600 + int(t[5]) * 60 + int(t[6])

# in Python2 exec is a keyword so one must use "exec_"
# but for Python3 we want to provide the nicer version "exec"
setattr(Pyboard, "exec", Pyboard.exec_)

def execfile(filename, device='/dev/ttyACM0', baudrate=115200, user='micro', password='python'):
    pyb = Pyboard(device, baudrate, user, password)
    pyb.enter_raw_repl()
    output = pyb.execfile(filename)
    stdout_write_bytes(output)
    pyb.exit_raw_repl()
    pyb.close()

def main():
    import argparse
    cmd_parser = argparse.ArgumentParser(description='Run scripts on the pyboard.')
    cmd_parser.add_argument('--device', default='/dev/ttyACM0', help='the serial device or the IP address of the pyboard')
    cmd_parser.add_argument('-b', '--baudrate', default=115200, help='the baud rate of the serial device')
    cmd_parser.add_argument('-u', '--user', default='micro', help='the telnet login username')
    cmd_parser.add_argument('-p', '--password', default='python', help='the telnet login password')
    cmd_parser.add_argument('-c', '--command', help='program passed in as string')
    cmd_parser.add_argument('-w', '--wait', default=0, type=int, help='seconds to wait for USB connected board to become available')
    cmd_parser.add_argument('--follow', action='store_true', help='follow the output after running the scripts [default if no scripts given]')
    cmd_parser.add_argument('files', nargs='*', help='input files')
    args = cmd_parser.parse_args()

    # open the connection to the pyboard
    try:
        pyb = Pyboard(args.device, args.baudrate, args.user, args.password, args.wait)
    except PyboardError as er:
        print(er)
        sys.exit(1)

    # run any command or file(s)
    if args.command is not None or len(args.files):
        # we must enter raw-REPL mode to execute commands
        # this will do a soft-reset of the board
        try:
            pyb.enter_raw_repl()
        except PyboardError as er:
            print(er)
            sys.exit(1)

        def execbuffer(buf):
            try:
                ret, ret_err = pyb.exec_raw(buf, timeout=None, data_consumer=stdout_write_bytes)
            except PyboardError as er:
                print(er)
                sys.exit(1)
            except KeyboardInterrupt:
                sys.exit(1)
            if ret_err:
                pyb.exit_raw_repl()
                pyb.close()
                stdout_write_bytes(ret_err)
                sys.exit(1)

        # run the command, if given
        if args.command is not None:
            execbuffer(args.command.encode('utf-8'))

        # run any files
        for filename in args.files:
            with open(filename, 'rb') as f:
                pyfile = f.read()
                execbuffer(pyfile)

        # exiting raw-REPL just drops to friendly-REPL mode
        pyb.exit_raw_repl()

    # if asked explicitly, or no files given, then follow the output
    if args.follow or (args.command is None and len(args.files) == 0):
        try:
            ret, ret_err = pyb.follow(timeout=None, data_consumer=stdout_write_bytes)
        except PyboardError as er:
            print(er)
            sys.exit(1)
        except KeyboardInterrupt:
            sys.exit(1)
        if ret_err:
            pyb.close()
            stdout_write_bytes(ret_err)
            sys.exit(1)

    # close the connection to the pyboard
    pyb.close()

if __name__ == "__main__":
    main()
