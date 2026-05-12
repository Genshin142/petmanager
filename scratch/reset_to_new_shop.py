import mysql.connector

db_config = {
    "host": "localhost",
    "user": "root",
    "password": "362345943",
    "database": "petstore"
}

def reset_to_new_shop():
    try:
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor()

        print("Starting business data reset...")
        cursor.execute("SET FOREIGN_KEY_CHECKS = 0")

        # 1. 清空业务流水表
        tables_to_truncate = [
            "orders",
            "appointments",
            "boarding_records",
            "logistics_tasks",
            "pet_activity_logs",
            "salary_records",
            "sys_performance_records",
            "sys_operation_logs"
        ]
        
        for table in tables_to_truncate:
            print(f"Truncating {table}...")
            cursor.execute(f"DELETE FROM {table}")
            cursor.execute(f"ALTER TABLE {table} AUTO_INCREMENT = 1")

        # 2. 重置会员财务数据
        print("Resetting member financial data...")
        cursor.execute("UPDATE members SET balance = 0, consume_amt = 0, points = 0, level_name = '普通会员'")

        # 3. 确保基础信息保留 (员工, 宠物, 商品, 服务)
        # 已经保留，脚本不做删除操作

        cursor.execute("SET FOREIGN_KEY_CHECKS = 1")
        conn.commit()
        print("Business data reset complete. Shop is now in 'Fresh Opening' state.")

    except mysql.connector.Error as err:
        print(f"Error: {err}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            cursor.close()
            conn.close()

if __name__ == "__main__":
    reset_to_new_shop()
