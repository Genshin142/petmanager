import mysql.connector

def sync_member_stats():
    conn = mysql.connector.connect(
        host="localhost",
        user="root",
        password="362345943",
        database="petstore"
    )
    cursor = conn.cursor(dictionary=True)
    
    print("====================================================")
    print("           开始历史订单与会员数据同步更新            ")
    print("====================================================")
    
    # 1. 查询所有有效会员（未逻辑删除的）
    cursor.execute("SELECT member_id, name, consume_amt, points FROM members WHERE is_deleted = 0")
    members = cursor.fetchall()
    
    updated_count = 0
    
    for m in members:
        member_id = m['member_id']
        name = m['name']
        old_consume = float(m['consume_amt'] or 0.0)
        old_points = int(m['points'] or 0)
        
        # 2. 查询该会员的所有已支付（Paid）订单金额之和
        cursor.execute(
            "SELECT SUM(actual_pay) as total_paid FROM orders WHERE member_id = %s AND status = 'Paid'", 
            (member_id,)
        )
        result = cursor.fetchone()
        
        new_consume = float(result['total_paid'] or 0.0)
        new_points = int(new_consume) # 1元积1分
        
        # 如果数据发生变化，进行更新
        if new_consume != old_consume or new_points != old_points:
            cursor.execute(
                "UPDATE members SET consume_amt = %s, points = %s WHERE member_id = %s",
                (new_consume, new_points, member_id)
            )
            print(f"会员 {name} (ID: {member_id}):")
            print(f"  累计消费: {old_consume:.2f} 元 -> {new_consume:.2f} 元")
            print(f"  可用积分: {old_points} -> {new_points}")
            print("----------------------------------------------------")
            updated_count += 1
            
    conn.commit()
    cursor.close()
    conn.close()
    
    print("====================================================")
    print(f"同步完成！共更新了 {updated_count} 位会员的数据。")
    print("====================================================")

if __name__ == "__main__":
    sync_member_stats()
