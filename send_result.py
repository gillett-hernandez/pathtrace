import os
from sendgrid import SendGridAPIClient
from sendgrid.helpers.mail import Mail, Attachment, FileContent, FileType, FileName, ContentId

message = Mail(
    from_email='pathtrace@example.com',
    to_emails='gillett.hernandez@gmail.com',
    subject='GitHub CI: Pathrace CI Render result',
    html_content='<img src="cid:myimagecid"/>')

file_path = 'out2.png'
with open(file_path, 'rb') as f:
    data = f.read()
encoded = base64.b64encode(data).decode()
attachment = Attachment()
attachment.file_content = FileContent(encoded)
attachment.file_type = FileType('png')
attachment.file_name = FileName('render.png')
attachment.disposition = Disposition('inline')
attachment.content_id = ContentId('myimagecid')

mail.add_attachment()


try:
    sg = SendGridAPIClient(os.environ.get('SENDGRID_API_KEY'))
    response = sg.send(message)
    print(response.status_code)
    print(response.body)
    print(response.headers)
except Exception as e:
    print(str(e))
