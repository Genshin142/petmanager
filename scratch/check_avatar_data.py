import mysql.connector
import base64

def check_image():
    try:
        conn = mysql.connector.connect(
            host="127.0.0.1",
            user="root",
            password="362345943",
            database="petstore"
        )
        cursor = conn.cursor()
        
        cursor.execute("SELECT pet_id, pet_name, avatar_path FROM pets WHERE pet_id = 1")
        row = cursor.fetchone()
        if not row:
            print("Pet not found")
            return
            
        pet_id, name, data = row
        print(f"Checking pet {pet_id} ({name})")
        
        if not data:
            print("No avatar data")
            return
            
        if data.startswith("data:image"):
            header, base64_str = data.split(",", 1)
            print(f"Header: {header}")
            print(f"Base64 length: {len(base64_str)}")
            
            try:
                img_data = base64.b64decode(base64_str)
                with open(f"scratch/pet_{pet_id}_check.jpg", "wb") as f:
                    f.write(img_data)
                print(f"Successfully saved to scratch/pet_{pet_id}_check.jpg")
            except Exception as e:
                print(f"Base64 decode failed: {e}")
        else:
            print("Data does not start with data:image")
            
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            conn.close()

if __name__ == "__main__":
    check_image()
