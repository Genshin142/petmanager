import mysql.connector
import urllib.request
import base64
import time

db_config = {
    "host": "localhost",
    "user": "root",
    "password": "362345943",
    "database": "petstore"
}

breed_urls = {
    "金毛": "https://images.pexels.com/photos/35439661/pexels-photo-35439661.jpeg?auto=compress&cs=tinysrgb&w=1200",
    "布偶猫": "https://images.pexels.com/photos/16513673/pexels-photo-16513673.jpeg?auto=compress&cs=tinysrgb&w=1200",
    "柯基": "https://images.pexels.com/photos/14730847/pexels-photo-14730847.jpeg?auto=compress&cs=tinysrgb&w=1200",
    "英短": "https://images.pexels.com/photos/9120707/pexels-photo-9120707.jpeg?auto=compress&cs=tinysrgb&w=1200",
    "柴犬": "https://images.pexels.com/photos/15585112/pexels-photo-15585112.jpeg?auto=compress&cs=tinysrgb&w=1200",
    "暹罗猫": "https://images.pexels.com/photos/33660000/pexels-photo-33660000.jpeg?auto=compress&cs=tinysrgb&w=1200",
    "比熊": "https://images.pexels.com/photos/17589407/pexels-photo-17589407.jpeg?auto=compress&cs=tinysrgb&w=1200",
    "拉布拉多": "https://images.pexels.com/photos/94829/pexels-photo-94829.jpeg?auto=compress&cs=tinysrgb&w=1200",
    "美短": "https://images.pexels.com/photos/13302239/pexels-photo-13302239.jpeg?auto=compress&cs=tinysrgb&w=1200",
    "垂耳兔": "https://images.pexels.com/photos/33285251/pexels-photo-33285251.jpeg?auto=compress&cs=tinysrgb&w=1200"
}

def download_and_convert(url):
    try:
        # Pexels 可能需要 User-Agent
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, timeout=30) as response:
            content = response.read()
            encoded_string = base64.b64encode(content).decode('utf-8')
            return f"data:image/jpeg;base64,{encoded_string}"
    except Exception as e:
        print(f"Error downloading {url}: {e}")
    return None

def update_pet_avatars():
    try:
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor()

        for breed, url in breed_urls.items():
            print(f"Processing breed: {breed}...")
            base64_data = download_and_convert(url)
            if base64_data:
                sql = "UPDATE pets SET avatar_path = %s WHERE breed = %s"
                cursor.execute(sql, (base64_data, breed))
                print(f"Successfully updated {cursor.rowcount} pets for breed {breed}")
            else:
                print(f"Failed to get data for breed {breed}")
            
            time.sleep(1)

        conn.commit()
        print("Pet avatars update complete.")

    except mysql.connector.Error as err:
        print(f"Database error: {err}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            cursor.close()
            conn.close()

if __name__ == "__main__":
    update_pet_avatars()
