import mysql.connector

def check_schema():
    try:
        conn = mysql.connector.connect(
            host="localhost",
            user="root",
            password="362345943",
            database="petstore"
        )
        cursor = conn.cursor()

        print("--- pets 表结构 ---")
        cursor.execute("DESC pets")
        for col in cursor.fetchall():
            print(col)

        print("\n--- orders 表结构 ---")
        cursor.execute("DESC orders")
        for col in cursor.fetchall():
            print(col)

        cursor.close()
        conn.close()
    except Exception as e:
        print(f"查询失败: {str(e)}")

if __name__ == "__main__":
    check_schema()
