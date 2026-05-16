import mysql.connector
from datetime import datetime

try:
    conn = mysql.connector.connect(
        host='localhost',
        user='root',
        password='362345943',
        database='petstore'
    )
    cursor = conn.cursor()
    
    order_no = 'TEST-' + datetime.now().strftime('%Y%m%d%H%M%S')
    sql = """INSERT INTO orders 
             (order_no, source_module, related_id, member_id, operator_id, 
              total_amount, discount, actual_pay, item_details, payment_method, status) 
             VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"""
    
    values = (order_no, 'Test', '1', 1, 1, 50.0, 1.0, 50.0, 'Test item', 'Cash', 'Paid')
    
    cursor.execute(sql, values)
    conn.commit()
    print(f"Insert successful, order_no: {order_no}")
    
except Exception as e:
    print(f"Error during insert: {e}")
finally:
    if 'conn' in locals() and conn.is_connected():
        conn.close()
