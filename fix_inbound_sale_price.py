import mysql.connector
from mysql.connector import Error

def fix_inbound_table():
    connection = None
    try:
        config = {
            'user': 'root',
            'password': '362345943', 
            'host': '127.0.0.1',
            'database': 'petstore'
        }

        connection = mysql.connector.connect(**config)
        if connection.is_connected():
            cursor = connection.cursor()

            # 检查 product_inbound 表的列
            print("Checking product_inbound table schema...")
            cursor.execute("DESC product_inbound")
            columns = [col[0] for col in cursor.fetchall()]

            if 'sale_price' not in columns:
                print("  + Adding 'sale_price' column to product_inbound...")
                # 将 sale_price 添加在 cost_price 之后
                cursor.execute("ALTER TABLE product_inbound ADD COLUMN sale_price DECIMAL(10, 2) DEFAULT 0.00 AFTER cost_price")
                connection.commit()
                print(">>> Success! 'sale_price' column added.")
            else:
                print(">>> 'sale_price' column already exists.")

    except Error as e:
        print(f"\n[ERROR] {e}")
    finally:
        if connection and connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    fix_inbound_table()
