import mysql.connector

def check_orders():
    try:
        conn = mysql.connector.connect(
            host="localhost",
            user="root",
            password="362345943",
            database="petstore"
        )
        cursor = conn.cursor(dictionary=True)

        print("--- 最近 20 条订单记录 ---")
        cursor.execute("SELECT order_no, source_module, total_amount, status, created_at FROM orders ORDER BY created_at DESC LIMIT 20")
        orders = cursor.fetchall()
        for order in orders:
            print(f"订单号: {order['order_no']}, 来源: {order['source_module']}, 金额: {order['total_amount']}, 状态: {order['status']}, 创建时间: {order['created_at']}")

        cursor.close()
        conn.close()
    except Exception as e:
        print(f"查询失败: {str(e)}")

if __name__ == "__main__":
    check_orders()
