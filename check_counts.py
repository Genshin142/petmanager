import mysql.connector

def check_counts():
    try:
        conn = mysql.connector.connect(
            host="localhost",
            user="root",
            password="362345943",
            database="petstore"
        )
        cursor = conn.cursor()

        cursor.execute("SELECT COUNT(*) FROM orders")
        print(f"订单总数: {cursor.fetchone()[0]}")

        cursor.execute("SELECT COUNT(*) FROM pets")
        print(f"宠物总数: {cursor.fetchone()[0]}")
        
        cursor.execute("SELECT COUNT(*) FROM boarding_rooms")
        print(f"房间总数: {cursor.fetchone()[0]}")

        cursor.close()
        conn.close()
    except Exception as e:
        print(f"查询失败: {str(e)}")

if __name__ == "__main__":
    check_counts()
