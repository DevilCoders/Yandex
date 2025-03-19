from flask import render_template, flash, redirect, url_for
from app import app
from app.forms import VlanToggler
from app.netbox_helper import NetboxInterface
from app.vlantoggler import Toggler


@app.route('/', methods=['GET', 'POST'])
def index():
    form = VlanToggler()
    if form.validate_on_submit():
        state = form.state.data
        tor = form.tor.data.replace(' ', '')
        intf = form.interface.data.lower().replace(' ', '')

        netbox_reply = NetboxInterface(tor, intf, state).interface_data
        if netbox_reply['code'] != 200:
            flash(netbox_reply['status'])
        else:
            netbox_reply['state'] = state

            with Toggler(netbox_reply) as t:
                result = t.toggle
            flash(result['status'])

        return redirect(url_for('index'))
    return render_template('index.html', form=form)
