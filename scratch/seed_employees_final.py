import mysql.connector
from datetime import date

db_config = {
    "host": "localhost",
    "user": "root",
    "password": "362345943",
    "database": "petstore"
}

def seed_complete_data():
    try:
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor()

        # 禁用外键检查彻底清理
        print("Cleaning up sys_employees table...")
        cursor.execute("SET FOREIGN_KEY_CHECKS = 0")
        cursor.execute("DELETE FROM sys_employees")
        cursor.execute("ALTER TABLE sys_employees AUTO_INCREMENT = 1")
        cursor.execute("SET FOREIGN_KEY_CHECKS = 1")

        # 使用真实姓名
        employees = [
            ("admin", "123456", "张震东", "店长", "管理部", "男", 35, "13800138001", "zhang@pet.com", "110101198901011234", 8000.00, 0.05, "张国庆", "13900139001", "北京市朝阳区宠物路1号", "本科"),
            ("li_beauty", "123456", "李曼妮", "美容师", "美容部", "女", 28, "13800138002", "li@pet.com", "110101199605052345", 6000.00, 0.15, "李建国", "13900139002", "上海市静安区萌宠街2号", "大专"),
            ("wang_star", "123456", "王志强", "美容师", "美容部", "男", 24, "13800138003", "wang@pet.com", "110101200010103456", 4000.00, 0.10, "王凤琴", "13900139003", "广州市天河区爱宠路3号", "大专"),
            ("zhao_cashier", "123456", "赵思悦", "收银", "前台部", "女", 22, "13800138004", "zhao@pet.com", "110101200202024567", 4500.00, 0.02, "赵德海", "13900139004", "深圳市南山区宠爱大道4号", "本科"),
            ("chen_driver", "123456", "陈嘉豪", "司机", "物流部", "男", 30, "13800138005", "chen@pet.com", "110101199403035678", 5000.00, 0.05, "陈玉兰", "13900139005", "成都市武侯区接送路5号", "高中"),
            ("sun_intern", "123456", "孙佳琪", "美容师", "美容部", "女", 20, "13800138006", "sun@pet.com", "110101200404046789", 3000.00, 0.08, "孙伟华", "13900139006", "杭州市西湖区学习路6号", "在读"),
        ]

        sql = """
        INSERT INTO sys_employees (
            username, password, real_name, role, department, gender, age, phone, 
            email, id_card, base_salary, commission_rate, emergency_contact, 
            emergency_phone, address, education, status, join_date, img_url, is_deleted
        ) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
        """

        for i, data in enumerate(employees):
            img_url = f"assets/avatars/emp_{i+1}.jpg"
            full_data = data + ('在职', date(2025, 1, 1), img_url, 0)
            cursor.execute(sql, full_data)

        conn.commit()
        print(f"Successfully re-seeded {cursor.rowcount} employees with full real names.")
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            cursor.close()
            conn.close()

if __name__ == "__main__":
    seed_complete_data()
