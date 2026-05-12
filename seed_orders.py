import mysql.connector
import random
from datetime import datetime, timedelta
import json

def seed_orders():
    try:
        conn = mysql.connector.connect(
            user='root',
            password='362345943',
            host='127.0.0.1',
            database='petstore'
        )
        cursor = conn.cursor(dictionary=True)

        print("Fetching existing data...")
        # 获取会员
        cursor.execute("SELECT member_id, name FROM members")
        members = cursor.fetchall()
        
        # 获取员工
        cursor.execute("SELECT emp_id, real_name as name FROM sys_employees")
        employees = cursor.fetchall()
        
        # 获取宠物
        cursor.execute("SELECT pet_id as id, pet_name as name FROM pets")
        pets = cursor.fetchall()
        
        # 获取商品
        cursor.execute("SELECT name, sale_price as price FROM products")
        products = cursor.fetchall()
        
        # 获取服务
        cursor.execute("SELECT name, price FROM services")
        services = cursor.fetchall()

        if not employees:
            print("Error: No employees found in sys_employees table. Please seed employees first.")
            return

        print("Cleaning old orders...")
        cursor.execute("SET FOREIGN_KEY_CHECKS = 0")
        cursor.execute("TRUNCATE TABLE orders")
        cursor.execute("SET FOREIGN_KEY_CHECKS = 1")

        print("Generating 800 random orders...")
        
        order_sql = """
            INSERT INTO orders (order_no, source_module, related_id, member_id, operator_id, total_amount, discount, actual_pay, item_details, payment_method, status, created_at)
            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
        """
        
        orders = []
        today = datetime.now()
        
        for i in range(800):
            # 随机日期 (过去1年)
            days_ago = random.randint(0, 365)
            created_at = today - timedelta(days=days_ago)
            created_at = created_at.replace(hour=random.randint(9, 21), minute=random.randint(0, 59))
            
            order_no = f"ORD{created_at.strftime('%Y%m%d%H%M%S')}{random.randint(100, 999)}"
            source_module = random.choice(['Product', 'Service', 'Foster', 'Logistics'])
            
            member = random.choice(members) if (members and random.random() > 0.3) else None
            member_id = member['member_id'] if member else None
            
            operator = random.choice(employees)
            operator_id = operator['emp_id']
            
            total_amount = 0
            item_details = ""
            related_id = ""
            
            if source_module == 'Product':
                if products:
                    count = random.randint(1, 4)
                    selected_prods = random.sample(products, min(count, len(products)))
                    total_amount = sum(float(p['price']) for p in selected_prods)
                    item_details = "+".join(p['name'] for p in selected_prods)
                else:
                    total_amount = random.randint(50, 200)
                    item_details = "精选宠物用品"
            
            elif source_module == 'Service':
                if services:
                    svc = random.choice(services)
                    total_amount = float(svc['price'])
                    item_details = svc['name']
                else:
                    total_amount = random.randint(30, 150)
                    item_details = "基础洗护服务"
            
            elif source_module == 'Foster':
                total_amount = random.randint(100, 1000)
                item_details = "寄养服务"
                related_id = str(random.randint(1000, 9999))
                
            elif source_module == 'Logistics':
                total_amount = random.randint(20, 80)
                item_details = "宠物接送服务"
                related_id = f"TASK{random.randint(1000, 9999)}"
                
            discount = random.choice([1.0, 1.0, 1.0, 0.95, 0.9, 0.88])
            actual_pay = total_amount * discount
            
            pay_method = random.choice(['MemberCard', 'Alipay', 'Wechat', 'Cash'])
            status = random.choices(['Paid', 'Unpaid', 'Cancelled'], weights=[90, 5, 5])[0]
            
            orders.append((
                order_no,
                source_module,
                related_id,
                member_id,
                operator_id,
                total_amount,
                discount,
                actual_pay,
                item_details,
                pay_method,
                status,
                created_at
            ))

        cursor.executemany(order_sql, orders)
        conn.commit()
        print(f">>> Success! 800 orders seeded into 'orders' table.")

    except mysql.connector.Error as err:
        print(f"Error: {err}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            cursor.close()
            conn.close()

if __name__ == "__main__":
    seed_orders()
