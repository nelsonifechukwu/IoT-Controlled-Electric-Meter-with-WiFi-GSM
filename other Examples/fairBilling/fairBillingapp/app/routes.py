from app import app 
from flask import session, make_response, redirect, render_template, request, flash, url_for, Response
from app.models import checklogin, delete_dist_data, insert_distribution_data, get_distribution_data, get_dist_state, update_dist_state, insert_power_data, get_unit, del_unitb, update_unitb, get_power_data 

API = 'xdol'

#login
@app.route('/', methods = ('GET', 'POST'))
@app.route('/login', methods = ('GET', 'POST'))
def login():
    if (request.method=='POST'):
        username = request.form['username']
        password = request.form['password']
        users = checklogin()
        if username == users[0][0] and password == users[0][1]:
            session['username'] = username
            flash('User Login Successful')
            return redirect(url_for('plot'))
        elif username == users[1][0] and password == users[1][1]:
            session['admin'] = username
            flash('Admin Login Successful')
            return redirect(url_for('meters'))
        else:
            flash('Error: Invalid Username or Password')
            return redirect(url_for('login'))
    else:
        return render_template('login.html')

#plot data
@app.route('/plot')
def plot():
    #create a session to secure this endpoint
    if "username" not in session:
        return redirect(url_for('login'))
    #data = get_transformer_data()
    return render_template('plot.html', name=session["username"])

#utility dashboard
@app.route('/utility')
def utility():
    #create a session to secure this endpoint
    if "admin" not in session:
        return redirect(url_for('login'))
    #data = get_transformer_data()
    return render_template('utility.html', name=session["admin"])

#meters view
@app.route('/meters')
def meters():
    #create a session to secure this endpoint
    if "admin" not in session:
        return redirect(url_for('login'))
    #data = get_transformer_data()
    return render_template('meters.html', name=session["admin"])


#design the web_api
@app.route('/update/key=<api_key>/c1=<int:c1>/v=<int:v>/ect=<int:ect>/ecm=<int:ecm>/unitl=<int:unitl>', methods=['GET', 'POST'])
def update(api_key, c1, v, ect, ecm, unitl):
    if (api_key == API):
        insert_distribution_data(c1, v)
        insert_power_data(ect, ecm, unitl)
        return Response(status=200)
    else:
        return Response(status=500)

#fetch data to be plotted
@app.route('/data', methods=['GET', 'POST'])
def get_data():
    data = get_distribution_data()
    data = str(data)
    data = data.strip('[]()')
    return str(data)

#get button state (for the hardware)
@app.route('/get_state', methods = ['GET'])
def get_state():
    dist_state = get_dist_state('all')
    dist_state = str(dist_state)
    dist_state = dist_state.strip('[](),')
    return str(dist_state)

@app.route('/get_unitb', methods = ['GET', 'POST'])
def unit():
    unit = get_unit()
    unit = str(unit).strip('[](),')
    return str(unit)

@app.route('/del_unitb', methods = ['GET', 'POST'])
def del_unit():
    del_unitb()
    return Response(status = 200)

# @app.route('/update_unitb/<int:unitb>', methods = ['GET', 'POST'])
# def update_unit(unitb):
#     print(unitb)
#     update_unitb(unitb) 
#     return Response(status = 200)

@app.route('/purchase/<int:b1>', methods = ['GET', 'POST'])
def purchase(b1):
    update_unitb(b1)
    return Response(status = 200)

@app.route('/get_power_data', methods = ['GET', 'POST'])
def power_data():
    power_data = get_power_data()
    power_data = str(power_data)
    power_data = power_data.strip('()[]').strip(',')
    return str(power_data)

#update button state
@app.route('/updatestate/button=<id>', methods = ['GET', 'POST'])
def update_state(id):
    #get the previous state
    state = get_dist_state(id)
    state =  str(state)
    #Strip the string from the database and update the previous state
    state = state.strip('()[]').strip(',')
    newstate =  1-int(state)
    update_dist_state(newstate, id)
    return str(newstate)

#delete all data
@app.route('/delete', methods = ('GET', 'POST'))
def delete():
    if request.method == 'POST': 
        delete_dist_data()
        insert_distribution_data(0,0)
        return redirect(url_for('plot'))

#get individual state
@app.route('/get_state/<id>', methods = ['GET'])
def get_id_state(id):
    dist_state = get_dist_state(id)
    dist_state = str(dist_state)
    dist_state = dist_state.strip('[](),')
    return str(dist_state)

#logout of session
@app.route('/logout', methods=['POST'])
def log_out():
    if "username" in session:
        session.pop("username")
    else:
        session.pop("admin")
    return redirect(url_for('login'))