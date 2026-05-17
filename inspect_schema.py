import mysql.connector

def inspect_schema():
    conn = mysql.connector.connect(
        host="localhost",
        user="root",
        password="362345943",
        database="petstore"
    )
    cursor = conn.cursor(dictionary=True)
    
    print("=== members Table Columns ===")
    cursor.execute("DESCRIBE members")
    for col in cursor.fetchall():
        print(f"{col['Field']}: {col['Type']} (Null: {col['Null']}, Key: {col['Key']}, Default: {col['Default']})")
        
    print("\n=== orders Table Columns ===")
    cursor.execute("DESCRIBE orders")
    for col in cursor.fetchall():
        print(f"{col['Field']}: {col['Type']} (Null: {col['Null']}, Key: {col['Key']}, Default: {col['Default']})")
        
    cursor.close()
    conn.close()

if __name__ == "__main__":
    inspect_schema()
