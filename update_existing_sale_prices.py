import mysql.connector
from mysql.connector import Error

def update_sale_prices():
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

            print("Updating existing sale_prices in product_inbound...")
            # 逻辑：拟定售价 = 成本价 * 1.5 (之前的系统默认加价率)
            update_query = """
                UPDATE product_inbound 
                SET sale_price = cost_price * 1.5 
                WHERE sale_price = 0 OR sale_price IS NULL
            """
            cursor.execute(update_query)
            affected_rows = cursor.rowcount
            connection.commit()
            
            print(f">>> Success! Updated {affected_rows} records with a 1.5x markup.")

            # 同时更新一下 products 表，确保同步
            print("Syncing sale_prices to products table where stock exists...")
            sync_query = """
                UPDATE products p
                JOIN (
                    SELECT barcode, MAX(sale_price) as latest_sale 
                    FROM product_inbound 
                    WHERE is_shelved = 1 
                    GROUP BY barcode
                ) i ON p.barcode = i.barcode
                SET p.sale_price = i.latest_sale
            """
            cursor.execute(sync_query)
            connection.commit()
            print(">>> Success! Products master table synced.")

    except Error as e:
        print(f"\n[ERROR] {e}")
    finally:
        if connection and connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    update_sale_prices()
