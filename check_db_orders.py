import mysql.connector
import json

def check_database():
    try:
        conn = mysql.connector.connect(
            host="localhost",
            user="root",
            password="362345943",
            database="petstore"
        )
        cursor = conn.cursor(dictionary=True)

        print("--- 最近 10 条订单记录 ---")
        # 增加 LIMIT 到 10 确保能看到刚才那一笔
        cursor.execute("SELECT * FROM orders ORDER BY created_at DESC LIMIT 10")
        orders = cursor.fetchall()
        for order in orders:
            print(f"订单号: {order['order_no']}, 来源: {order['source_module']}, 金额: {order['total_amount']}, 状态: {order['status']}, 创建时间: {order['created_at']}")

        print("\n--- 宠物状态核查 ---")
        # 使用正确的列名 pet_id, pet_name, current_status
        cursor.execute("SELECT pet_id, pet_name, current_status, room_no FROM pets WHERE pet_name = '大黄'")
        pet = cursor.fetchone()
        if pet:
            print(f"宠物: {pet['pet_name']}, ID: {pet['pet_id']}, 当前状态: {pet['current_status']}, 房号: {pet['room_no']}")
        else:
            print("未找到名为 '大黄' 的宠物")

        cursor.close()
        conn.close()
    except Exception as e:
        print(f"数据库查询失败: {str(e)}")

if __name__ == "__main__":
    check_database()
