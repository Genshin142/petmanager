import sqlite3 # Just to keep common imports, but we need subprocess for mysql cli
import subprocess
import os

# We will use the mysql CLI but pass the data carefully
mysql_bin = r"C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe"
user = "root"
password = "362345943"
database = "petstore"

sql_commands = [
    "SET NAMES utf8mb4;",
    "UPDATE orders SET item_details = '[{\"name\": \"全托普通房间\", \"barcode\": \"S001\", \"price\": 50.0, \"count\": 1}]' WHERE order_no = 'ORD-BO-20260513213646';",
    "UPDATE orders SET item_details = '[{\"name\": \"全托普通房间\", \"barcode\": \"S001\", \"price\": 50.0, \"count\": 1}]' WHERE order_no = 'ORD-BO-20260513210708';"
]

sql_script = "\n".join(sql_commands)

print("Running MySQL update via Python subprocess...")

try:
    # Use 'input' parameter of run() to send the SQL script as UTF-8 encoded bytes
    process = subprocess.run(
        [mysql_bin, "-u", user, f"-p{password}", "--default-character-set=utf8mb4", database],
        input=sql_script.encode('utf-8'),
        capture_output=True,
        check=True
    )
    print("Success!")
    print(process.stdout.decode('utf-8', errors='replace'))
except subprocess.CalledProcessError as e:
    print(f"Error: {e}")
    print(e.stderr.decode('utf-8', errors='replace'))
except Exception as e:
    print(f"Unexpected error: {e}")
