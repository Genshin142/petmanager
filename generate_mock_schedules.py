import mysql.connector
from datetime import datetime, timedelta
import random

def generate_schedules():
    db_config = {
        "host": "localhost",
        "user": "root",
        "password": "362345943",
        "database": "petstore"
    }
    
    try:
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor()
        
        # 清空 4/1 - 7/1 期间的原有排班，避免冲突
        cursor.execute("DELETE FROM sys_schedules WHERE work_date BETWEEN '2026-04-01' AND '2026-07-01'")
        
        start_date = datetime(2026, 4, 1)
        end_date = datetime(2026, 7, 1)
        
        # 员工 ID 列表
        emp_ids = [1, 2, 3, 4, 5, 6]
        
        curr_date = start_date
        while curr_date <= end_date:
            date_str = curr_date.strftime('%Y-%m-%d')
            weekday = curr_date.weekday() # 0 is Monday
            
            for emp_id in emp_ids:
                # 模拟休息安排
                is_off = False
                # 每个人每周轮流休息两天
                # 简单逻辑：固定某些人休周中，某些人休周末
                if emp_id == 1 and weekday in [5, 6]: # 店长周末双休 (偶尔加班)
                    if random.random() > 0.2: is_off = True
                elif emp_id in [2, 3] and weekday in [1, 2]: # 美容师 A/B 休周二三
                    is_off = True
                elif emp_id in [4, 6] and weekday in [3, 4]: # 前台和美容师 C 休周四五
                    is_off = True
                elif emp_id == 5 and weekday in [0, 6]: # 司机休周一和周日
                    is_off = True
                
                # 模拟请假 (1% 概率)
                note = ""
                if not is_off and random.random() < 0.02:
                    is_off = True
                    note = random.choice(["事假", "病假", "调休"])

                if is_off:
                    # 休息
                    sql = "INSERT INTO sys_schedules (emp_id, work_date, shift_type, plan_start, plan_end, note) VALUES (%s, %s, '休息', NULL, NULL, %s)"
                    cursor.execute(sql, (emp_id, date_str, note))
                else:
                    # 分配班次
                    # 早班: 09:00-18:00, 晚班: 13:00-22:00
                    if emp_id in [1, 5]: # 店长和司机基本早班
                        shift = "早班"
                        start, end = "09:00:00", "18:00:00"
                    else:
                        # 其他人轮班
                        if random.random() > 0.5:
                            shift = "早班"
                            start, end = "09:00:00", "18:00:00"
                        else:
                            shift = "晚班"
                            start, end = "13:00:00", "22:00:00"
                    
                    sql = "INSERT INTO sys_schedules (emp_id, work_date, shift_type, plan_start, plan_end) VALUES (%s, %s, %s, %s, %s)"
                    cursor.execute(sql, (emp_id, date_str, shift, start, end))
            
            curr_date += timedelta(days=1)
            
        conn.commit()
        print(f"成功生成 2026-04-01 至 2026-07-01 的模拟排班数据。")
        
    except mysql.connector.Error as err:
        print(f"错误: {err}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            cursor.close()
            conn.close()

if __name__ == "__main__":
    generate_schedules()
