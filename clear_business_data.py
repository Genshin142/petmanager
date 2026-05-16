import mysql.connector
import json

def clear_data():
    try:
        # 尝试从配置文件读取数据库连接信息 (根据您的项目结构推测)
        # 这里使用默认连接，如果您的密码不同请修改
        db_config = {
            "host": "localhost",
            "user": "root",
            "password": "362345943",
            "database": "petstore"
        }

        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor()

        print("正在清空业务数据...")

        # 禁用外键检查以允许清空
        cursor.execute("SET FOREIGN_KEY_CHECKS = 0;")

        tables_to_clear = [
            "orders",
            "appointments",
            "product_batches",
            "sys_performance_records",
            "salary_records",
            "pet_vaccine_records",
            "boarding_records"
        ]

        for table in tables_to_clear:
            try:
                cursor.execute(f"TRUNCATE TABLE {table};")
                print(f"  [OK] 已清空表: {table}")
            except mysql.connector.Error as err:
                print(f"  [SKIP] 跳过表 {table}: {err}")

        # 重置商品表的总库存为 0
        cursor.execute("UPDATE products SET stock_current = 0, stock_curr = 0;")
        print("  [OK] 已重置所有商品库存为 0")

        # 恢复外键检查
        cursor.execute("SET FOREIGN_KEY_CHECKS = 1;")

        conn.commit()
        print("\n业务数据已全部清空！您可以重新启动服务端和客户端开始测试。")

    except mysql.connector.Error as err:
        print(f"错误: {err}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            cursor.close()
            conn.close()

if __name__ == "__main__":
    clear_data()
