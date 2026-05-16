import mysql.connector

try:
    conn = mysql.connector.connect(
        host="localhost",
        user="root",
        password="362345943",
        database="petstore"
    )
    cursor = conn.cursor()
    cursor.execute("SELECT COLUMN_TYPE FROM information_schema.COLUMNS WHERE TABLE_NAME = 'sys_employees' AND COLUMN_NAME = 'status'")
    row = cursor.fetchone()
    if row:
        print(f"Status Column Type: {row[0]}")
        # Print hex of each part
        enum_str = row[0]
        # enum('val1','val2',...)
        vals = enum_str[5:-1].split("','")
        for v in vals:
            v = v.strip("'")
            print(f"Value: {v}, Hex: {v.encode('utf-8').hex()}")
            try:
                print(f"  GBK Hex: {v.encode('gbk').hex()}")
            except:
                pass
    
    cursor.execute("SELECT emp_id, real_name, status FROM sys_employees")
    for (eid, name, status) in cursor.fetchall():
        print(f"ID: {eid}, Name: {name}, Status: {status}, Hex: {status.encode('utf-8').hex()}")

except Exception as e:
    print(f"Error: {e}")
finally:
    if 'conn' in locals() and conn.is_connected():
        conn.close()
