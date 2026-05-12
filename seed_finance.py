import mysql.connector
from mysql.connector import Error
import random
from datetime import datetime, timedelta

def seed_finance():
    config = {
        'user': 'root',
        'password': '362345943', 
        'host': '127.0.0.1',
        'database': 'petstore'
    }

    try:
        connection = mysql.connector.connect(**config)
        if connection.is_connected():
            cursor = connection.cursor(dictionary=True)

            # 1. 确保表结构完整并修复编码问题 (扩展 sys_performance_records)
            print("Optimizing sys_performance_records schema...")
            # 强制重置 ENUM 以修复可能的乱码
            cursor.execute("ALTER TABLE sys_performance_records MODIFY COLUMN status ENUM('待核销', '已核销') DEFAULT '待核销'")
            cursor.execute("ALTER TABLE salary_records MODIFY COLUMN status ENUM('待发放', '已发放') DEFAULT '待发放'")
            
            cursor.execute("DESC sys_performance_records")
            columns = [col['Field'] for col in cursor.fetchall()]
            
            perf_fixes = {
                'customer_name': "VARCHAR(50) DEFAULT ''",
                'pay_method': "VARCHAR(20) DEFAULT 'MemberCard'",
                'pet_name': "VARCHAR(50) DEFAULT ''",
                'pet_breed': "VARCHAR(50) DEFAULT ''",
                'commission_type': "VARCHAR(20) DEFAULT '比例提成'",
                'commission_rate': "DECIMAL(10,2) DEFAULT 0.00"
            }
            
            for col, df in perf_fixes.items():
                if col not in columns:
                    print(f"  + Adding column: {col}...")
                    cursor.execute(f"ALTER TABLE sys_performance_records ADD COLUMN {col} {df}")

            # 2. 获取现有员工
            print("Fetching real employees...")
            cursor.execute("SELECT emp_id, real_name, base_salary FROM sys_employees WHERE is_deleted = 0")
            employees = cursor.fetchall()
            if not employees:
                print("Error: No employees found in sys_employees. Please seed employees first.")
                return

            # 3. 清理旧数据
            print("Cleaning old finance data...")
            cursor.execute("SET FOREIGN_KEY_CHECKS = 0;")
            cursor.execute("TRUNCATE TABLE sys_performance_records;")
            cursor.execute("TRUNCATE TABLE salary_records;")
            cursor.execute("SET FOREIGN_KEY_CHECKS = 1;")

            # 4. 生成模拟业绩数据
            print("Generating performance records...")
            perf_sql = """
                INSERT INTO sys_performance_records 
                (emp_id, order_id, service_date, service_name, order_amount, commission_amt, status, 
                 customer_name, pay_method, pet_name, pet_breed, commission_type, commission_rate) 
                VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
            """
            
            services = [
                ("全身深度洗护", 128.0, 40.0), # 固定 40
                ("全手剪精修造型", 388.0, 150.0), # 固定 150
                ("基础三项护理", 45.0, 15.0), # 固定 15
                ("寄养服务", 260.0, 39.0) # 固定 39
            ]
            
            customers = ["张三", "李芳", "王强", "赵六", "刘大"]
            pets = [("布丁", "金毛"), ("芝麻", "英短"), ("豆豆", "柴犬"), ("小雪", "萨摩耶")]
            
            today = datetime.now()
            perf_records = []
            
            for emp in employees:
                # 每个员工生成 15-25 条业绩 (覆盖过去两个月)
                num_records = random.randint(15, 25)
                for i in range(num_records):
                    svc_name, amount, rate = random.choice(services)
                    days_ago = random.randint(0, 60)
                    date = (today - timedelta(days=days_ago)).strftime('%Y-%m-%d')
                    status = "已核销" if days_ago > 10 else "待核销"
                    pet_name, pet_breed = random.choice(pets)
                    
                    perf_records.append((
                        emp['emp_id'],
                        f"ORD{date.replace('-', '')}{random.randint(1000, 9999)}",
                        date,
                        svc_name,
                        amount,
                        rate, # 这里的 rate 已经是固定金额了
                        status,
                        random.choice(customers),
                        random.choice(["MemberCard", "Alipay", "Wechat"]),
                        pet_name,
                        pet_breed,
                        "固定提成",
                        0.0
                    ))
            
            cursor.executemany(perf_sql, perf_records)

            # 5. 生成模拟薪资数据
            print("Generating salary records...")
            salary_sql = """
                INSERT INTO salary_records 
                (emp_id, salary_month, base_salary, commission, bonus, deduction, net_pay, status, pay_time, remark) 
                VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
            """
            
            months = ["2026-04", "2026-05"]
            salary_records = []
            
            for emp in employees:
                for month in months:
                    # 计算该月该员工的总提成
                    month_perf = [r for r in perf_records if r[0] == emp['emp_id'] and r[2].startswith(month)]
                    comm_sum = sum(r[5] for r in month_perf)
                    
                    base = emp['base_salary'] or 4000.0
                    bonus = random.choice([0, 0, 0, 200, 500])
                    deduction = random.choice([0, 0, 50, 100])
                    net = float(base) + float(comm_sum) + float(bonus) - float(deduction)
                    
                    status = "已发放" if month == "2024-04" else "待发放"
                    pay_time = f"{month}-10 10:00:00" if status == "已发放" else None
                    
                    salary_records.append((
                        emp['emp_id'],
                        month,
                        base,
                        comm_sum,
                        bonus,
                        deduction,
                        net,
                        status,
                        pay_time,
                        f"{month} 月份工资结算"
                    ))
            
            cursor.executemany(salary_sql, salary_records)

            connection.commit()
            print(">>> Success! Finance data seeded with real employees.")

    except Error as e:
        print(f"Error: {e}")
    finally:
        if connection and connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    seed_finance()
