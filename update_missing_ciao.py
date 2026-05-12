import mysql.connector
import base64
import os
import json

def image_to_base64(file_path):
    if not os.path.exists(file_path):
        print(f"Warning: File not found {file_path}")
        return ""
    with open(file_path, "rb") as image_file:
        return base64.b64encode(image_file.read()).decode('utf-8')

def update_ciao_grilled():
    db = mysql.connector.connect(
        host="localhost",
        user="root",
        password="362345943",
        database="petstore"
    )
    cursor = db.cursor()

    base_dir = r"C:\Users\任坤\.gemini\antigravity\brain\df9ce58e-49d6-4062-ba25-326214d12bdd"
    # This is the image generated earlier for 4901133718908
    image_file = os.path.join(base_dir, "ciao_grilled_bonito_views_1778425107092.png")
    barcode = "4901133718908"

    b64 = image_to_base64(image_file)
    if b64:
        img_data_val = json.dumps([b64])
        tables = ["products", "product_inbound"]
        for table in tables:
            sql = f"UPDATE {table} SET img_data = %s WHERE barcode = %s"
            cursor.execute(sql, (img_data_val, barcode))
            print(f"Updated {table} for {barcode}")
            
        db.commit()
        print("Success: Ciao Grilled Bonito images written to database.")
    else:
        print("Error: Could not find image file.")

    db.close()

if __name__ == "__main__":
    update_ciao_grilled()
