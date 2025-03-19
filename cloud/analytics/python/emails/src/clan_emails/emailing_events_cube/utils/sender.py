def get_sender_event(event):
    if event is None:
        return
    event = event.decode("utf-8") 
    if event == 'send':
        return 'email_sended'
    if event == 'px':
        return 'email_opened'
    if event == 'click':
        return 'email_clicked'
    if event == 'bounce':
        return 'unsub'
    return event
