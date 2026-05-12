import mysql.connector
import urllib.request
import os
from datetime import date

# 数据库配置
db_config = {
    "host": "localhost",
    "user": "root",
    "password": "362345943",
    "database": "petstore"
}

# 确保头像存储目录存在
avatar_dir = r"e:\QT\work\PetManager\assets\avatars"
if not os.path.exists(avatar_dir):
    os.makedirs(avatar_dir)

def download_avatar(emp_id, gender):
    """从网络下载随机头像并返回本地路径"""
    seed = emp_id + (100 if gender == "女" else 0)
    url = f"https://i.pravatar.cc/300?u={seed}"
    filename = f"emp_{emp_id}.jpg"
    path = os.path.join(avatar_dir, filename)
    db_path = f"assets/avatars/{filename}"
    
    try:
        print(f"Downloading avatar: {url} -> {path}")
        headers = {'User-Agent': 'Mozilla/5.0'}
        req = urllib.request.Request(url, headers=headers)
        with urllib.request.urlopen(req, timeout=15) as response:
            with open(path, "wb") as f:
                f.write(response.read())
        return db_path
    except Exception as e:
        print(f"Failed to download avatar: {e}")
    return ""

def seed_data():
    try:
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor()

        # 禁用外键检查进行清理
        print("Resetting employee table (disabling FK checks)...")
        cursor.execute("SET FOREIGN_KEY_CHECKS = 0")
        cursor.execute("DELETE FROM sys_employees")
        cursor.execute("ALTER TABLE sys_employees AUTO_INCREMENT = 1")
        cursor.execute("SET FOREIGN_KEY_CHECKS = 1")
        conn.commit()

        employees = [
            ("admin", "123456", "张店长", "店长", "管理部", "男", 35, "13800138001", 8000.00, 0.05),
            ("li_beauty", "123456", "李高级", "美容师", "美容部", "女", 28, "13800138002", 6000.00, 0.15),
            ("wang_star", "123456", "王助理", "美容师", "美容部", "男", 24, "13800138003", 4000.00, 0.10),
            ("zhao_cashier", "123456", "赵收银", "收银", "前台部", "女", 22, "13800138004", 4500.00, 0.02),
            ("chen_driver", "123456", "陈司机", "司机", "物流部", "男", 30, "13800138005", 5000.00, 0.05),
            ("sun_intern", "123456", "孙学徒", "美容师", "美容部", "女", 20, "13800138006", 3000.00, 0.08),
        ]

        sql = """
        INSERT INTO sys_employees (
            username, password, real_name, role, department, gender, age, phone, 
            base_salary, commission_rate, join_date, status, img_url
        ) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
        """

        for i, (uname, pwd, rname, role, dept, gender, age, phone, salary, comm) in enumerate(employees):
            join_date = date(2025, 1, 1)
            status = 1
            img_url = download_avatar(i + 1, gender)
            val = (uname, pwd, rname, role, dept, gender, age, phone, salary, comm, join_date, status, img_url)
            cursor.execute(sql, val)
            print(f"Successfully added employee: {rname} ({role})")

        conn.commit()
        print(f"Finished! Total {cursor.rowcount} records inserted.")
        
    except mysql.connector.Error as err:
        print(f"Database error: {err}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            cursor.close()
            conn.close()

if __name__ == "__main__":
    seed_data()
