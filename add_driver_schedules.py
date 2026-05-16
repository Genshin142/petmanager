import mysql.connector
from datetime import datetime, timedelta
import random

def add_driver_schedules():
    db_config = {
        "host": "localhost",
        "user": "root",
        "password": "362345943",
        "database": "petstore"
    }
    
    try:
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor()
        
        start_date = datetime(2026, 4, 1)
        end_date = datetime(2026, 7, 1)
        
        emp_id = 7 # 新司机 刘志远
        
        current_date = start_date
        while current_date <= end_date:
            date_str = current_date.strftime("%Y-%m-%d")
            
            # 策略：如果这天是周六或周日，概率上班
            # 如果是平时，轮休（陈嘉豪 E005 通常是固定休息，我们要确保两人不同时休）
            weekday = current_date.weekday() # 0 is Monday
            
            # 简单策略：E005 休周一，E007 就休周二
            if weekday == 1: # Tuesday
                shift_type = "休息"
                start_time = ""
                end_time = ""
            else:
                shift_type = "早班"
                start_time = "09:00"
                end_time = "18:00"
            
            query = "INSERT INTO sys_schedules (emp_id, work_date, shift_type, plan_start, plan_end) VALUES (%s, %s, %s, %s, %s)"
            cursor.execute(query, (emp_id, date_str, shift_type, start_time, end_time))
            
            current_date += timedelta(days=1)
            
        conn.commit()
        print(f"Successfully generated schedules for {emp_id} from 4/1 to 7/1")
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if conn.is_connected():
            cursor.close()
            conn.close()

if __name__ == "__main__":
    add_driver_schedules()
