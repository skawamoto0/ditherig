# Copyright (C) 2026 Suguru Kawamoto
# This software is released under the MIT License.

import asyncio
import hashlib
import hmac
from mcp.server import NotificationOptions, Server
from mcp.server.models import InitializationOptions, ServerCapabilities
import mcp.server.stdio
import mcp.types
import os
import time
import win32file
import win32pipe

def send(key, message):
    try:
        handle = win32file.CreateFile(r"\\.\pipe\ditherig.exe.api", win32file.GENERIC_READ | win32file.GENERIC_WRITE, 0, None, win32file.OPEN_EXISTING, 0, None)
        message = int((time.time() + 11644473600) * 10000000).to_bytes(8, "little") + len(message).to_bytes(4, "little") + message
        data = hmac.new(key, message, hashlib.sha256).digest() + message
        win32file.WriteFile(handle, data)
        _, data = win32file.ReadFile(handle, 4096, None)
        win32file.CloseHandle(handle)
        if len(data) >= 4 and int.from_bytes(data[:4], "little") != 0xffffffff:
            return True
    except:
        pass
    return False

async def main():
    server = Server("Dithering Settings for Integrated Graphics")
    algorithm_dict = {
        "none": 0,
        "spatial": 1,
        "spatio-temporal 1": 2,
        "spatio-temporal 2": 3,
        "temporal": 4
    }
    frequency_dict = {
        "default": 5,
        "highest": 6,
        "lowest": 7
    }
    @server.list_tools()
    async def list_tools() -> list[mcp.types.Tool]:
        return [
            mcp.types.Tool(
                name="ditherig_set_dithering_algorithm",
                description="Changes the dithering algorithm of the computer display through the \"Dithering Settings for Integrated Graphics\" application.",
                inputSchema={
                    "type": "object",
                    "properties": {
                        "algorithm": {
                            "type": "string",
                            "enum": list(algorithm_dict),
                            "description": "The dithering algorithm"
                        }
                    },
                    "required": ["algorithm"]
                }
            ),
            mcp.types.Tool(
                name="ditherig_set_gpu_frequency",
                description="Changes the GPU frequency of the computer through the \"Dithering Settings for Integrated Graphics\" application.",
                inputSchema={
                    "type": "object",
                    "properties": {
                        "frequency": {
                            "type": "string",
                            "enum": list(frequency_dict),
                            "description": "The GPU frequency"
                        }
                    },
                    "required": ["frequency"]
                }
            )
        ]
    @server.call_tool()
    async def handle_call_tool(name: str, arguments: dict) -> list[mcp.types.TextContent]:
        if name == "ditherig_set_dithering_algorithm":
            algorithm = algorithm_dict.get(arguments.get("algorithm"))
            if algorithm is not None:
                text = "Succeeded." if send(bytes.fromhex(os.environ.get("API_KEY")), algorithm.to_bytes(4, "little")) else "Failed."
                return mcp.types.TextContent(type="text", text=text)
        if name == "ditherig_set_gpu_frequency":
            frequency = frequency_dict.get(arguments.get("frequency"))
            if frequency is not None:
                text = "Succeeded." if send(bytes.fromhex(os.environ.get("API_KEY")), frequency.to_bytes(4, "little")) else "Failed."
                return mcp.types.TextContent(type="text", text=text)
        raise ValueError("")
    async with mcp.server.stdio.stdio_server() as (read_stream, write_stream):
        await server.run(read_stream, write_stream, InitializationOptions(server_name="Dithering Settings for Integrated Graphics", server_version="1.0.0", capabilities=ServerCapabilities(resources={})))

if __name__ == "__main__":
    asyncio.run(main())
