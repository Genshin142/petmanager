import mysql.connector

def check_orders():
    try:
        db = mysql.connector.connect(
            host="localhost",
            user="root",
            password="362345943",
            database="petstore"
        )
        cursor = db.cursor()
        
        print("Checking orders table...")
        cursor.execute("SELECT * FROM orders")
        rows = cursor.fetchall()
        print(f"Total rows in 'orders': {len(rows)}")
        for row in rows:
            print(row)
            
        print("\nChecking table structure:")
        cursor.execute("DESC orders")
        for col in cursor.fetchall():
            print(col)
            
        db.close()
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    check_orders()
