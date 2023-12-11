import sqlite3 as sql
from datetime import datetime

def checklogin():
    con = sql.connect("fairBilling.db")
    cur = con.cursor()
    cur.execute("SELECT username, password FROM users")
    users = cur.fetchall()
    con.commit()
    con.close()
    return users

def insert_distribution_data(c1, v):
    con = sql.connect("fairBilling.db")
    cur = con.cursor()
    #add datetime to the query
    cur.execute("INSERT INTO distribution_data(c1, v) VALUES (?,?)", (c1, v))# datetime.now()
    con.commit()
    con.close()

def insert_power_data(ect, ecm, unitl):
    consumption_update1 = f"UPDATE power_data SET ect = {ect} WHERE id = 1"
    consumption_update2 = f"UPDATE power_data SET ecm = {ecm} WHERE id = 1"
    unit_update = f"UPDATE power_data SET unitl = {unitl} WHERE id = 1"
    con = sql.connect("fairBilling.db") 
    cur = con.cursor()
    cur.execute(consumption_update1)
    cur.execute(consumption_update2)
    cur.execute(unit_update)
    con.commit()
    con.close()

def get_power_data():
    get = f"SELECT * FROM power_data"
    con = sql.connect("fairBilling.db")
    cur = con.cursor()
    cur.execute(get)
    data = cur.fetchall()
    con.commit()
    con.close()
    return data

def get_distribution_data():
    con = sql.connect("fairBilling.db")
    cur = con.cursor()
    #get the latest data
    cur.execute("SELECT * FROM distribution_data ORDER BY id DESC LIMIT 40")
    dist_data = cur.fetchall()
    con.commit()
    con.close()
    return dist_data

def get_unit():
    con = sql.connect("fairBilling.db")
    cur = con.cursor()
    get_unit = f"SELECT unitb FROM power_data WHERE id = 1"
    cur.execute(get_unit)
    detail = cur.fetchall()
    con.commit()
    con.close()
    return detail

def update_unitb(unitb: int):
    con = sql.connect("fairBilling.db")
    cur = con.cursor()
    update_unit = f"UPDATE power_data SET unitb = unitb + {unitb} WHERE id=1"
    cur.execute(update_unit)
    con.commit() 
    cur.close()

def del_unitb():
    con = sql.connect("fairBilling.db")
    cur = con.cursor()
    delete = f"UPDATE power_data SET unitb = 0 WHERE id=1"
    cur.execute(delete)
    con.commit()
    con.close()

def get_dist_state(type):
    con = sql.connect("fairBilling.db")
    cur = con.cursor()

    if type == 'all':
        cur.execute("SELECT c1_state FROM dist_state WHERE id = 1")
    else:
        cur.execute("SELECT {}_state FROM dist_state WHERE id = 1".format(type))

    dist_state = cur.fetchall()
    con.commit()
    con.close()
    return dist_state

def update_dist_state(state, type):
    column_name = f"{type}_state"
    value = int(state)
    query = f"UPDATE dist_state SET {column_name} = {value} WHERE id = 1"

    with sql.connect("fairBilling.db") as con:
        cur = con.cursor()
        cur.execute(query)
        con.commit()

def delete_dist_data():
    con = sql.connect("fairBilling.db")
    cur = con.cursor()
    cur.execute("DELETE FROM distribution_data")
    con.commit()
    con.close()

