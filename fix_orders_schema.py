import mysql.connector

def fix_orders_table():
    try:
        db = mysql.connector.connect(
            host="localhost",
            user="root",
            password="362345943",
            database="petstore"
        )
        cursor = db.cursor()
        
        print("Altering 'orders' table to fix 'status' column and collation...")
        
        # 1. Change status to VARCHAR to avoid enum issues
        cursor.execute("ALTER TABLE orders MODIFY COLUMN status VARCHAR(20) DEFAULT 'Unpaid'")
        
        # 2. Fix collation for the whole table just in case
        cursor.execute("ALTER TABLE orders CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci")
        
        # 3. Ensure order_no is VARCHAR(32) and unique
        # (It already is UNI based on previous DESC)
        
        db.commit()
        print("Successfully fixed orders table schema.")
        db.close()
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    fix_orders_table()
