import mysql.connector
import datetime

def main():
    try:
        conn = mysql.connector.connect(
            host="127.0.0.1",
            user="root",
            password="362345943",
            database="petstore",
            charset="utf8mb4"
        )
        cursor = conn.cursor()

        # 清空排班表
        print("Clearing sys_schedules table...")
        cursor.execute("TRUNCATE TABLE sys_schedules")

        # 准备一周的日期（从本周一开始）
        today = datetime.date.today()
        monday = today - datetime.timedelta(days=today.weekday())
        
        # 假设员工有 M00001(1) 到 M00003(3)
        # 插入模拟数据
        mock_data = [
            # 员工1 (ID: 1)
            (1, monday.strftime("%Y-%m-%d"), '早班', '09:00', '18:00'),
            (1, (monday + datetime.timedelta(days=1)).strftime("%Y-%m-%d"), '早班', '09:00', '18:00'),
            (1, (monday + datetime.timedelta(days=2)).strftime("%Y-%m-%d"), '休息', None, None),
            (1, (monday + datetime.timedelta(days=3)).strftime("%Y-%m-%d"), '晚班', '13:00', '22:00'),
            (1, (monday + datetime.timedelta(days=4)).strftime("%Y-%m-%d"), '晚班', '13:00', '22:00'),
            
            # 员工2 (ID: 2)
            (2, monday.strftime("%Y-%m-%d"), '晚班', '13:00', '22:00'),
            (2, (monday + datetime.timedelta(days=1)).strftime("%Y-%m-%d"), '休息', None, None),
            (2, (monday + datetime.timedelta(days=2)).strftime("%Y-%m-%d"), '早班', '09:00', '18:00'),
            (2, (monday + datetime.timedelta(days=3)).strftime("%Y-%m-%d"), '早班', '09:00', '18:00'),
            (2, (monday + datetime.timedelta(days=4)).strftime("%Y-%m-%d"), '休息', None, None),
            
            # 员工3 (ID: 3)
            (3, monday.strftime("%Y-%m-%d"), '休息', None, None),
            (3, (monday + datetime.timedelta(days=1)).strftime("%Y-%m-%d"), '晚班', '13:00', '22:00'),
            (3, (monday + datetime.timedelta(days=2)).strftime("%Y-%m-%d"), '晚班', '13:00', '22:00'),
            (3, (monday + datetime.timedelta(days=3)).strftime("%Y-%m-%d"), '早班', '09:00', '18:00'),
            (3, (monday + datetime.timedelta(days=4)).strftime("%Y-%m-%d"), '晚班', '13:00', '22:00'),
        ]

        query = """
            INSERT INTO sys_schedules (emp_id, work_date, shift_type, plan_start, plan_end)
            VALUES (%s, %s, %s, %s, %s)
        """
        
        cursor.executemany(query, mock_data)
        conn.commit()
        
        print(f"Successfully inserted {cursor.rowcount} mock schedule records.")
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if 'cursor' in locals() and cursor:
            cursor.close()
        if 'conn' in locals() and conn.is_connected():
            conn.close()

if __name__ == "__main__":
    main()
