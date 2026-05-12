import mysql.connector
import base64
import json
import urllib.request
from io import BytesIO

def download_and_update():
    url = "https://yanxuan-item.nosdn.127.net/0b7c0032b6ef985800d6239557238964.jpg"
    barcode = "6971161234567"
    
    print(f"Downloading image from {url}...")
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req) as response:
            data = response.read()
            b64_data = base64.b64encode(data).decode('utf-8')
        
        db = mysql.connector.connect(
            host="localhost",
            user="root",
            password="362345943",
            database="petstore"
        )
        cursor = db.cursor()
        
        img_data_val = json.dumps([b64_data])
        
        tables = ["products", "product_inbound"]
        for table in tables:
            sql = f"UPDATE {table} SET img_data = %s WHERE barcode = %s"
            cursor.execute(sql, (img_data_val, barcode))
            print(f"Updated {table} for {barcode}")
            
        db.commit()
        db.close()
        print("Success: Database updated with web image.")
        
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    download_and_update()
