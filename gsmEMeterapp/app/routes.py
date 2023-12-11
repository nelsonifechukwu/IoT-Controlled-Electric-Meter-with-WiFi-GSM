from app import app 
from flask import session, make_response, redirect, render_template, request, flash, url_for, Response
from app.models import checklogin, delete_dist_data, insert_distribution_data, get_distribution_data, get_dist_state, update_dist_state

API = 'xdol'

#login
@app.route('/', methods = ('GET', 'POST'))
@app.route('/login', methods = ('GET', 'POST'))
def login():
    if (request.method=='POST'):
        username = request.form['username']
        password = request.form['password']

        succeed = checklogin(username, password)
        if (succeed):
            session['username'] = username
            #global m_user
            #m_user = username
            flash('Welcome! Login Successful')
            return redirect(url_for('plot'))
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


#design the web_api
@app.route('/update/key=<api_key>/c1=<int:c1>/v=<int:v>', methods=['GET'])
def update(api_key, c1, v):
    if (api_key == API):
        insert_distribution_data(c1, v)
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
    session.pop("username")
    return redirect(url_for('login'))