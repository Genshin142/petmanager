import mysql.connector
from mysql.connector import Error

def fill_phones():
    connection = None
    try:
        config = {
            'user': 'root',
            'password': '362345943', 
            'host': '127.0.0.1',
            'database': 'petstore'
        }

        connection = mysql.connector.connect(**config)
        if connection.is_connected():
            cursor = connection.cursor()

            print("Filling missing supplier phones in database...")
            
            # 供应商映射表
            mappings = {
                '%派特%': '020-83569421',
                '%皇家%': '400-820-2233',
                '%比乐%': '021-57683211',
                '%福贝%': '021-57683211',
                '%合众众诚%': '010-62118888',
                '%渴望%': '010-65824133',
                '%佩蒂%': '0577-63852112',
                '%网易%': '0571-89852112',
                '%伊纳宝%': '0532-86123456',
                '%阿卡娜%': '010-65824133',
                '%魏斯宝%': '400-600-1234'
            }

            total_updated = 0
            for pattern, phone in mappings.items():
                # 更新 product_inbound
                cursor.execute("""
                    UPDATE product_inbound 
                    SET supplier_phone = %s 
                    WHERE (supplier_phone IS NULL OR supplier_phone = '' OR supplier_phone = '-')
                    AND supplier LIKE %s
                """, (phone, pattern))
                total_updated += cursor.rowcount
                
                # 更新 products
                cursor.execute("""
                    UPDATE products 
                    SET supplier_phone = %s 
                    WHERE (supplier_phone IS NULL OR supplier_phone = '' OR supplier_phone = '-')
                    AND supplier LIKE %s
                """, (phone, pattern))
                total_updated += cursor.rowcount

            connection.commit()
            print(f">>> Success! Filled phone numbers for {total_updated} records.")

    except Error as e:
        print(f"\n[ERROR] {e}")
    finally:
        if connection and connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    fill_phones()
