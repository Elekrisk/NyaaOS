import json
import pty
from queue import Queue
from select import select
import socket
import string
import subprocess
import threading
from time import sleep
import lldb
import os



# with open("IMAGE_BASE", "w"):
#     pass

try:
    with open("log", "w") as f:
        print("Log:", file=f)

        tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        def qemu():
            master, slave = pty.openpty()
            process = subprocess.Popen(["make", "run-debug"], text=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
            while True:
                if process.poll() != None:
                    os._exit(1)
                readable, _, _ = select([process.stderr, process.stdout], [], [], 5)
                for ff in readable:
                    line: str = ff.readline()
                    if line == "":
                        continue;
                    print(line, end='', file=f)


        qemu_thread = threading.Thread(target=qemu)
        qemu_thread.start()

        while True:
            try:
                print("Trying to connect...", file=f)
                tcp_socket.connect(('localhost', 12345))
                break
            except Exception as e:
                print("Error: " + str(e), file=f)
                sleep(1)

        # while True:
        #     with open("IMAGE_BASE", "r") as f:
        #         contents = filter(lambda x: x.strip() != "", f.readlines())
        #     line = next(contents, "").strip()
        #     if line != "":
        #         try:
        #             res = int(line, base=0)
        #             break
        #         except:
        #             pass
        #     sleep(1.0)

        # res = int(input("Base: "), base=0)

        def get_debugger() -> lldb.SBDebugger:
            return lldb.debugger

        result = lldb.SBCommandReturnObject()

        debugger = get_debugger()
        # ci: lldb.SBCommandInterpreter = debugger.GetCommandInterpreter()

        # ci.HandleCommand("gdb-remote 1234", result)
        debugger.HandleCommand("gdb-remote 1234")
        # ci.HandleCommand("continue", result)
        debugger.HandleCommand("continue")


        buffer = ""
        cont = True
        while cont:
            msg = str(tcp_socket.recv(1024), encoding="utf-8")
            buffer += msg
            lines = buffer.split("\n")
            buffer = lines[-1]
            for line in lines[:-1]:
                print(f"Examining line {line}", file=f)
                if line.startswith("!!"):
                    res = int(line.lstrip("!").strip(), base=0)
                    cont = False
                    break

        # ci.HandleCommand("\x03", result)
        debugger.HandleCommand("\x03")
        target: lldb.SBTarget = debugger.GetSelectedTarget()
        print(target, file=f)
        module: lldb.SBModule = target.module["bootloader-debug.efi"]
        print(module, file=f)

        addr: lldb.SBAddress = module.GetObjectFileHeaderAddress()

        base_addr = addr.file_addr
        for section in module.section_iter():
            section: lldb.SBSection
            addr: lldb.SBAddress = section.addr
            diff: int = addr.file_addr - base_addr
            section_addr = res + diff
            result = lldb.SBCommandReturnObject()
            command_string = f"ta m load -f {module.file.fullpath} \"{section.name}\" {hex(section_addr)}"
            print(command_string, file=f)
            # ci.HandleCommand(command_string, result)
            debugger.HandleCommand(command_string)

        tcp_socket.send(b'\n')

except Exception as e:
    with open("log", "a") as f:
        print("Error: " + str(e), file=f)