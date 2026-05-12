import mysql.connector
from mysql.connector import Error

def fix_collations():
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

            print("Unifying collations to utf8mb4_unicode_ci...")
            
            # 修改表的字符集
            tables = ['products', 'product_inbound']
            for table in tables:
                print(f"  + Fixing table: {table}...")
                cursor.execute(f"ALTER TABLE {table} CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci")
            
            connection.commit()
            print(">>> Success! Collations unified.")

    except Error as e:
        print(f"\n[ERROR] {e}")
    finally:
        if connection and connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    fix_collations()
