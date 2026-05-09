import mysql.connector
from mysql.connector import Error

def audit_database():
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
            
            # 获取所有表
            cursor.execute("SHOW TABLES")
            tables = [t[0] for t in cursor.fetchall()]
            
            print("="*50)
            print(f" DATABASE AUDIT: {config['database']}")
            print("="*50)

            for table in tables:
                print(f"\n[Table: {table}]")
                cursor.execute(f"DESC {table}")
                cols = cursor.fetchall()
                for col in cols:
                    print(f"  - {col[0]}: {col[1]}")

            print("\n" + "="*50)

    except Error as e:
        print(f"Error: {e}")
    finally:
        if connection and connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    audit_database()
