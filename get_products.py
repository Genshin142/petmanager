import mysql.connector

def get_products():
    db = mysql.connector.connect(
        host="localhost",
        user="root",
        password="362345943",
        database="petstore"
    )
    cursor = db.cursor()
    cursor.execute("SELECT barcode, name FROM products WHERE is_active = 1")
    products = cursor.fetchall()
    db.close()
    return products

if __name__ == "__main__":
    products = get_products()
    for barcode, name in products:
        print(f"{barcode}|{name}")
