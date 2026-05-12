import mysql.connector
from datetime import date, timedelta
import random

db_config = {
    "host": "localhost",
    "user": "root",
    "password": "362345943",
    "database": "petstore"
}

def seed_data():
    try:
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor()

        # 清理数据
        print("Cleaning up members and pets tables...")
        cursor.execute("SET FOREIGN_KEY_CHECKS = 0")
        cursor.execute("DELETE FROM pets")
        cursor.execute("DELETE FROM members")
        cursor.execute("ALTER TABLE members AUTO_INCREMENT = 1")
        cursor.execute("ALTER TABLE pets AUTO_INCREMENT = 1")
        cursor.execute("SET FOREIGN_KEY_CHECKS = 1")

        # 准备会员数据
        member_names = ["陈大志", "林美惠", "黄小明", "吴淑芬", "李建宏", "张丽华", "王志文", "徐美玲", "孙伟杰", "周晓彤"]
        genders = ["男", "女", "男", "女", "男", "女", "男", "女", "男", "女"]
        phones = [f"13{random.randint(100000000, 999999999)}" for _ in range(10)]
        
        member_sql = """
        INSERT INTO members (phone, name, gender, birthday, balance, consume_amt, points, level_name, pay_pwd)
        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)
        """
        
        # 准备宠物数据
        pet_names = ["大黄", "雪球", "肉丸", "可乐", "年糕", "豆包", "奶酪", "布丁", "奥利奥", "糯米"]
        species_list = ["狗", "猫", "狗", "猫", "狗", "猫", "狗", "狗", "猫", "兔"]
        breeds = ["金毛", "布偶猫", "柯基", "英短", "柴犬", "暹罗猫", "比熊", "拉布拉多", "美短", "垂耳兔"]
        pet_genders = ["公", "母", "绝育公", "绝育母", "未知", "公", "母", "公", "母", "未知"]

        for i in range(10):
            # 插入会员
            birthday = date(1985 + random.randint(0, 20), random.randint(1, 12), random.randint(1, 28))
            balance = random.uniform(500, 5000)
            consume = random.uniform(0, 2000)
            points = int(consume / 10)
            level = "黄金会员" if balance > 2000 else "普通会员"
            
            cursor.execute(member_sql, (phones[i], member_names[i], genders[i], birthday, balance, consume, points, level, "123456"))
            member_id = cursor.lastrowid
            
            # 插入宠物
            age = random.randint(3, 60)
            weight = random.uniform(2, 30)
            
            pet_sql = """
            INSERT INTO pets (member_id, pet_name, species, breed, gender, age_months, weight, health_status, current_status)
            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)
            """
            cursor.execute(pet_sql, (member_id, pet_names[i], species_list[i], breeds[i], pet_genders[i], age, weight, "健康", "在店" if i % 3 == 0 else "在家"))

        conn.commit()
        print("Successfully seeded 10 members and 10 pets.")

    except mysql.connector.Error as err:
        print(f"Error: {err}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            cursor.close()
            conn.close()

if __name__ == "__main__":
    seed_data()
